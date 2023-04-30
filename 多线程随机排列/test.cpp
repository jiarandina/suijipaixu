#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

std::mutex mtx;
std::atomic_bool sorted(false);

bool is_sorted(const std::vector<std::atomic_int*>& arr) {
	for (size_t i = 1; i < arr.size(); ++i) {
		if (arr[i - 1]->load() > arr[i]->load()) {
			return false;
		}
	}
	return true;
}

void print_array(const std::vector<std::atomic_int*>& arr, int i = 0) {
	std::unique_lock<std::mutex> lock(mtx);
	std::cout << "Thread id:" << i << ",";
	for (const auto& num : arr) {
		std::cout << " " << num->load();
	}
	std::cout << std::endl;
}

void shuffle_and_check(std::vector<std::atomic_int*>& arr, int id) {
	std::random_device rd;
	std::mt19937 g(rd());
	std::lock_guard<std::mutex> lock(mtx);
	while (!sorted.load()) {
		for (size_t i = 0; i < arr.size(); ++i) {
			size_t j = std::uniform_int_distribution<size_t>(0, arr.size() - 1)(g);
			if (i != j) {
				int tmp = arr[i]->exchange(arr[j]->load());
				arr[j]->store(tmp);
			}
		}
		if (is_sorted(arr)) {
			sorted.store(true);
		}
	}
}




std::vector<std::atomic_int*> init_atomic_vector(const std::vector<int>& init_arr) {
	std::vector<std::atomic_int*> arr;
	for (const auto& num : init_arr) {
		std::atomic_int* tmp = new std::atomic_int(num);
		arr.push_back(tmp);
	}
	return arr;
}

int main() {
	std::vector<int> init_arr = { 3, 1, 4, 1, 5, 9, 5 };
	std::vector<std::atomic_int*> arr = init_atomic_vector(init_arr);
	//const unsigned num_threads = std::thread::hardware_concurrency();
	const unsigned num_threads = 16;
	std::vector<std::thread> threads;

	auto startTime = std::chrono::high_resolution_clock::now();
	for (unsigned i = 0; i < num_threads; ++i) {
		threads.push_back(std::thread([&, i]() { shuffle_and_check(arr, i); }));
	}

	for (auto& th : threads) {
		th.join();
	}
	auto stopTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stopTime - startTime);

	std::cout << "sec: " << duration.count() / 1000000 << "s Sorted array: ";
	print_array(arr);

	for (auto& num : arr) {
		delete num;
	}

	return 0;
}

