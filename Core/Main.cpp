// MiniThing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "MiniThing.h"
#include "../Utility/Utility.h"

#include <random>
#include "../ThreadPool/ThreadPool.h"

#if 1

int main()
{
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeEnd;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;

    QueryPerformanceCounter(&timeStart);

    std::wstring folderName;
    std::cout << "Which folder ? (eg. C:)" << std::endl;
    std::wcin >> folderName;
    MiniThing* pMiniThing = new MiniThing(folderName, ".\\MiniThing.db");

    QueryPerformanceCounter(&timeEnd);
    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
    std::cout << "Time elapsed : " << elapsed << " S" << std::endl;

    if (FAILED(pMiniThing->CreateMonitorThread()))
    {
        assert(0);
    }
    if (FAILED(pMiniThing->CreateQueryThread()))
    {
        assert(0);
    }
    pMiniThing->StartMonitorThread();
    pMiniThing->StartQueryThread();

    Sleep(1000 * 1200);

    pMiniThing->StopMonitorThread();
    pMiniThing->StopQueryThread();
}

#else

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);


void simulate_hard_computation() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << std::endl;
}

// Same as before but now we have an output parameter
void multiply_output(int& out, const int a, const int b) {
    simulate_hard_computation();
    out = a * b;
    std::cout << a << " * " << b << " = " << out << std::endl;
}

// Same as before but now we have an output parameter
int multiply_return(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << std::endl;
    return res;
}


int main(int argc, char* argv[])
{
    // Create pool with 3 threads
    ThreadPool pool(10);

    // Initialize pool
    pool.init();

    // Submit (partial) multiplication table
    for (int i = 1; i < 10; ++i) {
        for (int j = 1; j < 10; ++j) {
            pool.submit(multiply, i, j);
        }
    }

    // Submit function with output parameter passed by ref
    int output_ref;
    auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

    // Wait for multiplication output to finish
    future1.get();
    std::cout << "Last operation result is equals to " << output_ref << std::endl;

    // Submit function with return parameter
    auto future2 = pool.submit(multiply_return, 5, 3);

    // Wait for multiplication output to finish
    int res = future2.get();
    std::cout << "Last operation result is equals to " << res << std::endl;

    pool.shutdown();

    return 0;
}

#endif