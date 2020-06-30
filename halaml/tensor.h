#ifndef TENSOR_H
#define TENSOR_H
#include <memory>
#include <numeric>
#include <vulkan/vulkan.h>
#include "madml.h"

namespace kernel
{
	class buffer;

	class tensor
	{
	public:
		tensor(Format fmt = kFormatFp32);
		tensor(char* data, std::vector<int> shape, Format fmt = kFormatInvalid);
		tensor(float c, std::vector<int> shape, Format fmt = kFormatFp32);
		void* map();
		void unMap();
		Shape getShape() const;
		int getId() const;
		int dimNum() const;
		int dimSize(int axis) const;
		int count(int start_axis = 0, int end_axis = -1) const;
		char* toHost();
		tensor reshape(const char* data, const std::vector<int>& shape, bool alloc = false,
			Format fmt = kFormatInvalid);
		tensor reshape(const std::vector<int>& shape);
		void setTo(float val);
		Format getFormat() const;
		size_t size() const { return size_in_byte; }
		bool isEmpty() const { return size_in_byte == 0; }
		void copyTo(tensor dst);
		std::shared_ptr<buffer>& getBuffer() { return m_buffer; }

	private:

		int id;
		bool counted = false;
		VkDevice m_device;
		std::vector<int> m_shape;
		size_t size_in_byte;
		std::shared_ptr<char> m_data;
		std::shared_ptr<buffer> m_buffer;
		Format format;
		static int& get_object_id();
		void update_id();
	};
}

template <typename dType = float>
char* fill_memory_shape(std::vector<int> shape, dType c)
{
	size_t _shape = std::accumulate(std::begin(shape), std::end(shape), 1, std::multiplies<int>());
	dType* ret = new dType[_shape];
	for (int i = 0; i < _shape; ++i)
		ret[i] = reinterpret_cast<dType&>(c);
	return reinterpret_cast<char*>(ret);
}

#endif