#include "common.h"
#include "utils.h"
#include "activation.h"
#define LOCAL_SZ_X 1024
#define MAX_COMPUTE_WORK_GROUP_COUNT 65535

namespace kernel
{
	namespace layers
	{
		activation_fn::activation_fn(float alpha, bool in_place) : unary_operator(in_place)
		{
			m_param.alpha = alpha;
		}

		namespace activation
		{
			celu::celu(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "celu";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& celu::hook(const std::shared_ptr<tensor>& x)
			{
				alpha = std::make_shared<tensor>(tensor(1.0, x->getShape(), Format::kFormatFp32));
				return layer_construct_forward<activation_param>(shaders::celu_spv, sizeof(shaders::celu_spv), x, alpha, m_param);
			}

			elu::elu(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "elu";
				bck_shader = shaders::d_elu_spv;
				bck_codeSize = sizeof(shaders::d_elu_spv);
			}

			std::shared_ptr<tensor>& elu::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::elu_spv, sizeof(shaders::elu_spv), x, m_param);
			}

			hardshrink::hardshrink(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "hardshrink";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& hardshrink::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::hardshrink_spv, sizeof(shaders::hardshrink_spv), x, m_param);
			}

			hardtanh::hardtanh(float min_val, float max_val, bool in_place) :
				activation_fn(0, in_place), m_param({ 0, min_val, max_val })
			{
				m_type = "hardtanh";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& hardtanh::hook(const std::shared_ptr<tensor>& x)
			{
				m_param.total = x->count();
				return layer_construct_forward<two_param>(shaders::hardshrink_spv, sizeof(shaders::hardshrink_spv), x, m_param);
			}

			leakyrelu::leakyrelu(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "leakyrelu";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& leakyrelu::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::leakyrelu_spv, sizeof(shaders::leakyrelu_spv), x, m_param);
			}

			logsigmoid::logsigmoid(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "logsigmoid";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& logsigmoid::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::logsigmoid_spv, sizeof(shaders::logsigmoid_spv), x, m_param);
			}

			prelu::prelu(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "prelu";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& prelu::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::prelu_spv, sizeof(shaders::prelu_spv), x, m_param);
			}

			relu::relu(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "relu";
				bck_shader = shaders::d_relu_spv;
				bck_codeSize = sizeof(shaders::d_relu_spv);
			}

			std::shared_ptr<tensor>& relu::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::relu_spv, sizeof(shaders::relu_spv), x, m_param);
			}

			relu6::relu6(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "relu6";
				bck_shader = shaders::d_relu6_spv;
				bck_codeSize = sizeof(shaders::d_relu6_spv);
			}

			std::shared_ptr<tensor>& relu6::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::relu6_spv, sizeof(shaders::relu6_spv), x, m_param);
			}

			selu::selu(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "selu";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& selu::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::selu_spv, sizeof(shaders::selu_spv), x, m_param);
			}

			sigmoid::sigmoid(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "sigmoid";
				bck_shader = shaders::d_sigmoid_spv;
				bck_codeSize = sizeof(shaders::d_sigmoid_spv);
			}

			std::shared_ptr<tensor>& sigmoid::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::sigmoid_spv, sizeof(shaders::sigmoid_spv), x, m_param);
			}

			softplus::softplus(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "softplus";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& softplus::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::softplus_spv, sizeof(shaders::softplus_spv), x, m_param);
			}

			softshrink::softshrink(float alpha, bool in_place) : activation_fn(alpha, in_place)
			{
				m_type = "softshrink";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& softshrink::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::softshrink_spv, sizeof(shaders::softshrink_spv), x, m_param);
			}

			softsign::softsign(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "softsign";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& softsign::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::softsign_spv, sizeof(shaders::softsign_spv), x, m_param);
			}

			tanhshrink::tanhshrink(bool in_place) : activation_fn(0, in_place)
			{
				m_type = "tanhshrink";
				bck_shader = shaders::d_celu_spv;
				bck_codeSize = sizeof(shaders::d_celu_spv);
			}

			std::shared_ptr<tensor>& tanhshrink::hook(const std::shared_ptr<tensor>& x)
			{
				return layer_construct_forward<activation_param>(shaders::tanhshrink_spv, sizeof(shaders::tanhshrink_spv), x, m_param);
			}
		}
	}
}