#ifndef MATMUL_H
#define MATMUL_H

#include "madml.h"
#include "layer.h"

namespace kernel
{
	namespace layers
	{
		struct matmul_param
		{
			int total;
			int m;
			int n;
			int k;
		};

		class matmul : public Base_Layer<matmul_param>
		{
		private:
			void computeGroupCount() override;
		public:
			matmul();
			std::shared_ptr<tensor>& hook(const std::shared_ptr<tensor>& x, const std::shared_ptr<tensor>& w);
		};
	}
}

#endif
