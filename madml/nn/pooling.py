from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from typing import Union, List, Optional

import numpy as np

from madml import tensor, zeros
from .module import Module
from .testing import maxpool_forward, maxpool_backward
from .transform import vol2col
import vknn

MAX_DIMS = 3

def _dim_fix(arr, arg_arr, pi):
    def parse(x):
        return [x for _ in range(pi)] if isinstance(x, int) else [x[t] for t in range(pi)]

    if isinstance(arg_arr, int):
        arg_arr = parse(arg_arr)
    j = 0
    for i in range(len(arg_arr) - 1, len(arr)):
        arr[i] = arg_arr[j]
        j += 1
    return arr

class _MaxPoolNd(Module):
    __constants__ = ['kernel_size', 'stride', 'padding', 'dilation', 'return_indices', 'ceil_mode']

    return_indices : bool
    ceil_mode : bool

    def __init__(self, dims, kernel_size: Union[int, List[int]], stride: Union[int, List[int]]=None,
                 padding: Union[int, List[int]]=0, dilation: Union[int, List[int]]=1, return_indices: bool=False,
                 ceil_mode: bool=False) -> None:
        super(_MaxPoolNd, self).__init__()
        self.use_gpu = True
        self.dims = dims
        self.kernel_size = _dim_fix([1 for _ in range(MAX_DIMS)], kernel_size, dims)
        self.stride = _dim_fix([1 for _ in range(MAX_DIMS)], stride, dims)
        self.padding = _dim_fix([0 for _ in range(MAX_DIMS)], padding, dims)
        self.dilation = _dim_fix([1 for _ in range(MAX_DIMS)], dilation, dims)
        self.return_indices = return_indices
        self.ceil_mode = ceil_mode
        self.channel_offset = 1
        self._col = []
        self._vol = []
        self.batch_size = 0
        self.in_channels = 0
        self.col = None
        self.vol_col = None
        self.max_idx = []

    def setup_forward(self, x: tensor) -> None:
        if self.vol_col is None:
            self.batch_size = x.shape[0]
            self.in_channels = x.shape[1]
            self._col = [1 for _ in range(MAX_DIMS)]
            self._vol = [1 for _ in range(MAX_DIMS)]
            for i in range(1, self.dims + 1):
                self._col[-i] = int((x.shape[-i] + 2 * self.padding[-i] - self.dilation[-i] * (self.kernel_size[-i] - 1) - 1) // self.stride[-i]) + 1
                self._vol[-i] = x.shape[-i]
                self.channel_offset *= self.kernel_size[i]
            self.vol_col = vol2col(self.batch_size, self.in_channels, self._vol, self._col, self.kernel_size, self.stride, self.padding, self.dilation)
            self.output_shape = [self._col[i] for i in range(-1, -(self.dims + 1), -1)]
            self.y = zeros([self.batch_size, self.in_channels, *self.output_shape])
            out_size = np.prod(self.output_shape)
            max_idx_size = self.in_channels * self.batch_size * out_size
            self.max_idx = tensor([0 for i in range(max_idx_size)], [self.in_channels * self.batch_size, out_size], dtype=int)
            self.pool_kernel = vknn.max_reduce(self.in_channels, self.batch_size, False)
            self.pool_kernel_dcol = vknn.max_reduce(self.in_channels, self.batch_size, True)

    def forward_cpu(self, x: tensor) -> tensor:
        self.setup_forward(x)
        col = self.vol_col.forward_cpu(x)
        col.reshape([self.in_channels * self.batch_size, self.channel_offset, -1])

        y = self.y.host_data
        y = y.reshape([self.in_channels * self.batch_size, -1])
        for i in range(self.in_channels * self.batch_size):
            tmp = col.host_data[i]
            m_idx = np.argmax(tmp, axis=0)
            self.max_idx.host_data[i] = m_idx
            tmp = col.host_data[i][m_idx, range(m_idx.size)]
            y[i] = tmp

        self.y.host_data = y
        self.y.reshape([self.batch_size, self.in_channels, *self.output_shape])

        self.register_backward_arg('x', x)
        self.register_backward_arg('y', self.y)
        self.register_backward_arg('col', col)

        return self.y

    def forward_gpu(self, x: tensor) -> tensor:
        self.setup_forward(x)
        col = self.vol_col.forward_gpu(x)
        col.reshape([self.in_channels * self.batch_size, self.channel_offset, -1])
        col.download()

        self.y.reshape([self.in_channels * self.batch_size, -1])
        self.pool_kernel.forward(self.y.device_data, col.device_data, self.max_idx.device_data)
        self.y.reshape([self.batch_size, self.in_channels, *self.output_shape])

        self.register_backward_arg('x', x)
        self.register_backward_arg('y', self.y)
        self.register_backward_arg('col', col)

        return self.y

    def backward_cpu(self, x: tensor, y: tensor, col: tensor) -> tensor:
        dx, dy, dcol = x.gradient, y.gradient, col.gradient
        dcol.reshape([self.in_channels * self.batch_size, self.channel_offset, -1])
        dy_col = dy.host_data
        dy_col = dy_col.reshape([self.in_channels * self.batch_size, -1])

        d_col = dcol.host_data
        for i in range(self.in_channels * self.batch_size):
            m_idx = self.max_idx.host_data[i]
            d_col[i][m_idx, range(m_idx.size)] = dy_col[i]

        dcol.host_data = d_col
        _dx = self.vol_col.backward_cpu()
        return dx

    def backward_gpu(self, x: tensor, y: tensor, col: tensor) -> tensor:
        dx, dy, dcol = x.gradient, y.gradient, col.gradient
        dcol.reshape([self.in_channels * self.batch_size, self.channel_offset, -1])
        dy.reshape([self.in_channels * self.batch_size, -1])
        self.pool_kernel_dcol.forward(dy.device_data, dcol.device_data, self.max_idx.device_data)
        self.pool_kernel_dcol.run()
        _dx = self.vol_col.backward_gpu()
        return dx

    def print_l(self) -> None:
        x, y, _ = self.cache
        super(_MaxPoolNd, self).print_l()
        print('\tmax input:', x.host_data.max(), 'g', x.gradient.host_data.max(),
              ' output:', y.host_data.max(), 'g', y.gradient.host_data.max())
        print('\tmin input:', x.host_data.min(), 'g', x.gradient.host_data.min(),
              ' output:', y.host_data.min(), 'g', y.gradient.host_data.min())

class maxpool1d(_MaxPoolNd):
    kernel_size : int
    stride : int
    padding : int
    dilation : int

    def __init__(self, kernel_size: int, stride: Optional[int]=None,
                 padding: int=0, dilation: int=1,
                 return_indices: bool=False, ceil_mode: bool=False) -> None:
        super(maxpool1d, self).__init__(1, kernel_size, stride, padding, dilation, return_indices, ceil_mode)

class maxpool2d(_MaxPoolNd):
    kernel_size : Union[int, List[int]]
    stride : Union[int, List[int]]
    padding : Union[int, List[int]]
    dilation : Union[int, List[int]]

    def __init__(self, kernel_size: Union[int, List[int]], stride: Optional[Union[int, List[int]]]=None,
                 padding: Union[int, List[int]]=0, dilation: Union[int, List[int]]=1,
                 return_indices: bool=False, ceil_mode: bool=False) -> None:
        super(maxpool2d, self).__init__(2, kernel_size, stride, padding, dilation, return_indices, ceil_mode)

class maxpool3d(_MaxPoolNd):
    kernel_size : Union[int, List[int]]
    stride : Union[int, List[int]]
    padding : Union[int, List[int]]
    dilation : Union[int, List[int]]

    def __init__(self, kernel_size: Union[int, List[int]], stride: Optional[Union[int, List[int]]]=None,
                 padding: Union[int, List[int]]=0, dilation: Union[int, List[int]]=1,
                 return_indices: bool=False, ceil_mode: bool=False) -> None:
        super(maxpool3d, self).__init__(3, kernel_size, stride, padding, dilation, return_indices, ceil_mode)