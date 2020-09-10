#ifndef LAYER_H
#define LAYER_H
#include "backend.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdarg>
#include <future>
#include <list>

constexpr int local_sz_x = 1024;
constexpr int max_compute_work_group_count = 1024;

namespace layers
{
	class Module;
}

class context;

struct operator_param
{
	int total;
};

class layer
{
public:
	layer();
	virtual ~layer();
	void initVulkanThing(int buffer_num_forward);

	void createDescriptorSetLayout(int buffer_num);
	void createDescriptorSet(int buffer_num);
	void createShaderModule(const uint32_t* spv, size_t sz, const std::string& source = std::string());
	void createPipeline(size_t push_constants_size = 0, VkSpecializationInfo* specialization_info = nullptr);
	void createCommandBuffer();
	void recordCommandBuffer(void* push_constants = nullptr, size_t push_constants_size = 0) const;
	int runCommandBuffer();
	void bindTensor(std::shared_ptr<tensor> tensor, int binding);

	virtual void computeGroupCount() = 0;
	VkDevice m_device;

	VkPipeline m_pipeline;
	VkCommandBuffer m_cmd_buffer;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_pipeline_layout;
	VkShaderModule m_module;

	friend class layers::Module;

	int m_group_x;
	int m_group_y;
	int m_group_z;
};

namespace layers
{
	class Module : std::enable_shared_from_this<Module>
	{
	public:
		virtual void update_weight();

		std::shared_ptr<Module> getptr();
		std::shared_ptr<tensor> dx, dy, dw, db;
		virtual int set_backward() { return -1; }

		//std::vector<std::future<int>>& get_futures();
		int get_id() const;
		Module* m1 = nullptr;
		Module* m2 = nullptr;

	protected:
		std::string m_type;

		static std::vector<Module*>& get_module();
		static bool& sub_graph_bit();
		static void set_sub_graph();
		static void unset_sub_graph();

		static bool& train();
		static void eval();

		static void zero_grad();
		static void add_module(Module* M);

		Module* get_input_id(size_t i);

		int id = 0;
		static int& get_object_id();
		void update_id();

		//		std::vector<std::future<int>> m_futures;

		std::shared_ptr<tensor> x, y, w, b, t1, t2, t3, t4;

	private:

		//void BFS(std::vector<std::vector<int>> adj, int s = 0);
	};
}

template <class T = operator_param>
class Base_Layer : public layer, public layers::Module
{
public:
	Base_Layer(int forward_buffers, bool in_place = false);
	int set_backward() override;
	bool is_bias = false;
protected:
	const uint32_t* bck_shader;
	size_t bck_codeSize;
	Base_Layer* derivative;

	bool m_in_place;
	T m_param;
	void computeGroupCount() override;
	bool group_set = false;
	void set_group(int x, int y, int z);

public:
	std::shared_ptr<tensor>& layer_construct_forward(const uint32_t* shader, size_t codeSize,
		const std::shared_ptr<tensor>& x, Format fmt = Format::kFormatFp32,
		std::vector<int> output_shape = {});
	std::shared_ptr<tensor>& layer_construct_forward(const uint32_t* shader, size_t codeSize,
		const std::shared_ptr<tensor>& x, const std::shared_ptr<tensor>& w,
		Format fmt = Format::kFormatFp32, std::vector<int> output_shape = {});
};

template <typename T>
Base_Layer<T>::Base_Layer(int forward_buffers, bool in_place) : m_in_place(in_place), m_param({ 0 })
{
	update_id();
	if (!sub_graph_bit())
		add_module(this);
	bck_shader = nullptr;
	bck_codeSize = 0;
	derivative = nullptr;
	initVulkanThing(forward_buffers);
}

template <typename T>
void Base_Layer<T>::computeGroupCount()
{
	if (!group_set)
	{
		size_t sz = m_param.total;
		int n = local_sz_x;
		m_group_x = ((sz + n - 1) & -n) / n;
		if (m_group_x > max_compute_work_group_count)
			m_group_x = max_compute_work_group_count;
	}
	m_group_y = 1;
	m_group_z = 1;
}

template <typename T>
void Base_Layer<T>::set_group(int x, int y, int z)
{
	m_group_x = x;
	m_group_y = y;
	m_group_z = z;
	group_set = true;
}

template <typename T>
int Base_Layer<T>::set_backward()
{
	if (!dy)
		dy = std::make_shared<tensor>(tensor(0.0, y->getShape()));
	if (!dw && w)
		dw = std::make_shared<tensor>(tensor(0.0, w->getShape()));
	if (!db && b)
		db = std::make_shared<tensor>(tensor(0.0, b->getShape()));

	if (is_bias)
	{
		return dy->getId();
	}
	if (bck_codeSize)
	{
		if (train() && m2)
		{
			dx = derivative->layer_construct_forward(bck_shader, bck_codeSize, dy, dw, Format::kFormatFp32, x->getShape());
			m2->dy = dw;
		}
		else
			dx = derivative->layer_construct_forward(bck_shader, bck_codeSize, dy, Format::kFormatFp32, x->getShape());
		m1->dy = dx;
	}
	return dy->getId();
}

template <typename T>
std::shared_ptr<tensor>& Base_Layer<T>::layer_construct_forward(const uint32_t* shader, size_t codeSize,
	const std::shared_ptr<tensor>& _x, Format fmt,
	std::vector<int> output_shape)
{
	x = _x;
	float* t = (float*)x->toHost();
	if (!y)
	{
		if (output_shape.size() != 0)
			y = std::make_shared<tensor>(tensor(0.0, output_shape, fmt));
		else
			y = std::make_shared<tensor>(tensor(0.0, x->getShape()));
	}

	if (m_pipeline == nullptr)
	{
		m_param.total = x->count();
		computeGroupCount();
		createShaderModule(shader, codeSize);
		createPipeline(sizeof(T));
	}

	bindTensor(x, 0);
	bindTensor(y, 1);

	m1 = get_input_id(x->getId());

	if (train() && bck_codeSize && !derivative)
	{
		derivative = new Base_Layer<T>(2, false);
		derivative->m_param = m_param;
		derivative->set_group(m_group_x, m_group_y, m_group_z);
	}

	recordCommandBuffer(static_cast<void*>(&m_param), sizeof(T));
	runCommandBuffer();

	return y;
}

template <typename T>
std::shared_ptr<tensor>& Base_Layer<T>::layer_construct_forward(const uint32_t* shader, size_t codeSize,
	const std::shared_ptr<tensor>& _x,
	const std::shared_ptr<tensor>& _w, Format fmt,
	std::vector<int> output_shape)
{
	x = _x;
	w = _w;

	if (!y)
	{
		if (output_shape.size() != 0)
			y = std::make_shared<tensor>(tensor(0.0, output_shape, fmt));
		else
			y = std::make_shared<tensor>(tensor(0.0, x->getShape()));
	}

	if (m_pipeline == nullptr)
	{
		m_param.total = x->count();
		computeGroupCount();
		createShaderModule(shader, codeSize);
		createPipeline(sizeof(T));
	}

	bindTensor(x, 0);
	bindTensor(w, 1);
	bindTensor(y, 2);

	m1 = get_input_id(x->getId());
	m2 = get_input_id(w->getId());

	if (train() && bck_codeSize && !derivative)
	{
		derivative = new Base_Layer<T>(3, false);
		derivative->m_param = m_param;
		derivative->set_group(m_group_x, m_group_y, m_group_z);
	}

	recordCommandBuffer(static_cast<void*>(&m_param), sizeof(T));
	runCommandBuffer();

	return y;
}

#endif