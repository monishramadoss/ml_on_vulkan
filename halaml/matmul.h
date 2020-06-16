#ifndef MATMUL_H
#define MATMUL_H

#include "madml.h"
#include "layer.h"

namespace kernel {
	namespace layers {
		class matmul : public layer, public Module
		{
		public:
			matmul();			
			bool forward(tensor* x, tensor* y, tensor* z);
			void reshapeOutTensor(tensor* x, tensor* z);
			bool forward(std::vector<tensor*>& ins, std::vector<tensor*>& outs);
			void backward(){}
			void update_weight() {};
			bool operator()(tensor* x, tensor* y) { return false; };

		
		private:
			bool computeGroupCount();
			int m_m;
			int m_n;
			int m_k;

			static std::vector<Module*> module_list;
			virtual std::vector<Module*>* get_module();
		};

	}
}

#endif
