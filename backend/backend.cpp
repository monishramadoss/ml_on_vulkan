#include <memory.h>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <future>
#include <thread>

#include "backend.h"
#include "test.h"

using namespace std::chrono;

//#define TEST_TRANS
//#define TEST_MATH
//#define TEST_MEMORY

//#define TEST_NN

#define TEST_CNN

//#define TEST_RNN

//#define TEST_MNIST

void test_fn()
{
#ifdef TEST_MEMORY
	std::cout << "testing memory" << std::endl;
	{
		const std::vector<int> shape_x{ 512,512,512,4 };
		auto t1 = std::make_shared<tensor>(tensor(1.0, shape_x));
		std::vector<double> toHost, toDevice;
		for (int i = 0; i < 10; ++i)
		{
			std::cout << '\r' << i << " toHost ";
			auto start = std::chrono::system_clock::now();
			char* data = t1->toHost();
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> seconds = end - start;
			toHost.push_back(seconds.count());
			std::cout << seconds.count() << " toDevice ";
			std::this_thread::sleep_for(5s);
			start = std::chrono::system_clock::now();
			t1->toDevice(data);
			end = std::chrono::system_clock::now();
			seconds = end - start;
			toDevice.push_back(seconds.count());
			std::cout << seconds.count();
			std::this_thread::sleep_for(5s);
		}
		double avg1 = 0;
		double avg2 = 0;
		for (int i = 0; i < toHost.size(); ++i)
		{
			avg1 += toHost[i];
			avg2 += toDevice[i];
		}
		std::cout << "\nAvg toHost: " << avg1 / toHost.size() << " Avg toDevice: " << avg2 / toDevice.size() << std::endl;
	}
#endif

#ifdef TEST_TRANS
	std::cout << "testing trans_op" << std::endl;
	{
		const std::vector<int> shape_x{ 2, 3, 4 };
		char* dat = init::normal_distribution_init(shape_x, 20, 2);
		auto t1 = std::make_shared<tensor>(tensor(dat, shape_x));
		auto k1 = layers::transpose(std::vector<int>{ 1, 2, 0});
		auto t2 = k1.operator()(t1);

		test::PrintDiffer(reinterpret_cast<float*>(t1->toHost()), t1->count());
		std::cout << "\n\n\n";
		test::PrintDiffer(reinterpret_cast<float*>(t2->toHost()), t2->count());
	}
#endif

#ifdef TEST_MATH
	std::cout << "testing add_op" << std::endl;
	{
		const std::vector<int> shape_x{ 2000 };
		auto t1 = std::make_shared<tensor>(tensor(-6.0, shape_x));
		auto t2 = std::make_shared<tensor>(tensor(-1.0, shape_x));
		auto k1 = layers::math::add();
		auto k2 = layers::math::abs();
		auto k3 = layers::math::abs();

		auto t4 = k2(t2);
		auto t5 = k3(t1);
		auto t3 = k1(t4, t5);
		test::PrintDiffer(reinterpret_cast<float*>(t1->toHost()), t1->count());
		test::PrintDiffer(reinterpret_cast<float*>(t4->toHost()), t4->count());
		test::PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());
	}
#endif

#ifdef TEST_NN
	std::cout << "testing dnn" << std::endl;
	{
		const int M = 256;
		const int K = 256;
		const int N = 256;
		const std::vector<int> shape_x{ M, K };
		auto t1 = std::make_shared<tensor>(tensor(1.0, shape_x));
		auto layer1 = layers::nn::dense(N, false);
		auto layer2 = layers::nn::dense(N, false);
		auto layer3 = layers::nn::dense(N, false);
		auto layer4 = layers::nn::dense(N, false);

		std::vector<double> toHost;
		auto start = std::chrono::system_clock::now();
		auto t3 = layer1(t1);
		auto t4 = layer2(t3);
		auto t5 = layer3(t4);
		auto t6 = layer4(t5);
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> seconds = end - start;
		double first_latency = seconds.count();
		test::PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());
		for (int i = 0; i < 100; ++i)
		{
			start = std::chrono::system_clock::now();

			t3 = layer1(t1);
			t4 = layer2(t3);
			t5 = layer3(t4);
			t6 = layer4(t5);

			end = std::chrono::system_clock::now();
			seconds = end - start;
			toHost.push_back(seconds.count());
			std::cout << '\r' << i << " compute " << seconds.count();
		}

		std::cout << std::endl << "Fist Latency " << first_latency;
		std::cout << " Avg " << std::accumulate(toHost.begin(), toHost.end(), 0.0) / toHost.size();
		std::cout << " Max " << *std::max_element(toHost.begin(), toHost.end());
		std::cout << " Min " << *std::min_element(toHost.begin(), toHost.end()) << std::endl;

		//auto y_true = std::make_shared<tensor>(tensor(1.0, t4->getShape()));
		//loss(y_true, t4);
		//loss.backward();
		test::PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());

		//test::PrintMatrix(reinterpret_cast<float*>(t3->toHost()), t3->getShape());

		//test::PrintDiffer(reinterpret_cast<float*>(t4->toHost()), t4->count());
		//test::PrintDiffer(reinterpret_cast<float*>(t5->toHost()), t5->count());
		//test::PrintDiffer(reinterpret_cast<float*>(t6->toHost()), t6->count());

		std::cin.get();
	}
#endif
#ifdef TEST_CNN
	std::cout << "testing cnn" << std::endl;
	{
		//cdhw
		std::vector<int> shape_x{ 1, 1, 1, 5, 5 };
		std::vector<int> shape_y{ 1, 1, 1, 3, 3 };
		std::vector<int> shape_z{ 1, 1, 1, 7, 5 };
		auto* data1 = init::fill_memory_iter(shape_x);
		auto* data2 = init::fill_memory_iter(shape_y);
		auto* data3 = init::fill_memory_iter(shape_z);
		auto t1 = std::make_shared<tensor>(tensor(data1, shape_x));
		auto t2 = std::make_shared<tensor>(tensor(data2, shape_y));
		auto t11 = std::make_shared<tensor>(tensor(data3, shape_z));
		auto cnn_layer_1 = layers::nn::conv(1, { 1,3,3 }, { 1,1,1 }, { 0,1,1 }, { 1,1,1 }, 0, false);
		auto cnn_layer_1_1 = layers::nn::conv(1, { 1,3,3 }, { 1,2,2 }, { 0,1,1 }, { 1,1,1 }, 0, false);
		auto cnn_layer_2 = layers::nn::convTranspose(2, { 1,3,3 }, { 1,1,1 }, { 0,0,0 }, { 1,1,1 }, 0, false);

		auto t3 = cnn_layer_1(t1);
		std::cout << "input" << std::endl;
		std::cout << *t1;
		std::cout << "output" << std::endl;
		std::cout << *t3;
		auto t31 = cnn_layer_1_1(t11);
		std::cout << "input" << std::endl;
		test::PrintMatrix(reinterpret_cast<float*>(t11->toHost()), t11->getShape());
		std::cout << "output" << std::endl;
		test::PrintMatrix(reinterpret_cast<float*>(t31->toHost()), t31->getShape());
		auto t4 = cnn_layer_2(t2);
		std::cout << "input" << std::endl;
		test::PrintMatrix(reinterpret_cast<float*>(t2->toHost()), t2->getShape());
		std::cout << "output" << std::endl;
		test::PrintMatrix(reinterpret_cast<float*>(t4->toHost()), t4->getShape());
		std::cin.get();
	}
#endif
#ifdef TEST_RNN
	int length = 4;
	int vocab = 16;
	int num_layers = 4;
	int hidden_size = 128;

	std::cout << "testing rnn" << std::endl;
	{
		std::vector<int> shape_x{ length, vocab };
		auto t1 = std::make_shared<tensor>(tensor(1, shape_x));
		auto rnn_layer_1 = layers::nn::RNN(vocab, hidden_size, num_layers, length, false);
		auto tup = rnn_layer_1(t1);

		auto t3 = std::get<0>(tup);
		auto t4 = std::get<1>(tup);

		PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());
		PrintDiffer(reinterpret_cast<float*>(t4->toHost()), t4->count());
		std::cout << std::endl;
	}

	std::cout << "testing lstm" << std::endl;
	{
		std::vector<int> shape_x{ length, vocab };
		auto t1 = std::make_shared<tensor>(tensor(1, shape_x));
		auto rnn_layer_1 = layers::nn::LSTM(vocab, hidden_size, num_layers, length, false);
		auto tup = rnn_layer_1(t1);

		auto t3 = std::get<0>(tup);
		auto t4 = std::get<1>(tup);
		auto t5 = std::get<2>(tup);

		test::PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());
		test::PrintDiffer(reinterpret_cast<float*>(t4->toHost()), t4->count());
		test::PrintDiffer(reinterpret_cast<float*>(t5->toHost()), t5->count());
		std::cout << std::endl;
	}

	std::cout << "testing gru" << std::endl;
	{
		std::vector<int> shape_x{ length, vocab };
		auto t1 = std::make_shared<tensor>(tensor(1, shape_x));
		auto rnn_layer_1 = layers::nn::GRU(vocab, hidden_size, num_layers, length, true);
		auto tup = rnn_layer_1(t1);

		auto t3 = std::get<0>(tup);
		auto t4 = std::get<1>(tup);

		test::PrintDiffer(reinterpret_cast<float*>(t3->toHost()), t3->count());
		test::PrintDiffer(reinterpret_cast<float*>(t4->toHost()), t4->count());
		std::cout << std::endl;
	}
#endif
#ifdef TEST_MNIST
	std::cout << "testing mnist" << std::endl;
	{
		auto l1 = layers::nn::dense(64, true);
		auto l2 = layers::activation::relu();
		auto l3 = layers::nn::dense(64, true);
		auto l4 = layers::activation::relu();
		auto l5 = layers::nn::dense(10, true);
		auto l6 = layers::activation::relu();
		auto l7 = layers::math::add();
		auto loss_fn = loss::MSE();

		auto t0 = std::make_shared<tensor>(tensor(-0.5, std::vector<int>{3, 1, 784}));
		auto y_true = std::make_shared<tensor>(tensor(1, std::vector<int>{3, 1, 10}));
		auto t1 = l1(t0);
		auto t2 = l2(t1);
		auto t3 = l3(t2);
		auto t4 = l4(t3);
		auto tx = l7(t2, t4);
		auto t5 = l5(tx);
		auto t6 = l6(t5);

		loss_fn(t6, y_true);
		test::PrintDiffer(reinterpret_cast<float*>(t6->toHost()), t6->count());
	}

#endif
}

PYBIND11_MODULE(backend, m)
{
	m.doc() = "pybind11 testing pipeline"; // optional module docstring
	m.def("test", &test_fn, "A function which tests ml pipeline");
#ifdef VERSION_INFO
	m.attr("__version__") = VERSION_INFO;
#else
	m.attr("__version__") = "dev";
#endif
}