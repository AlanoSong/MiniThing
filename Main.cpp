// MiniThing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "MiniThing.h"
#include "Utility/Utility.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

int main()
{
    MiniThing* pMiniThing = new MiniThing(L"F:", ".\\test.db");

    // Create monitor thread to record file change
    if (FAILED(pMiniThing->CreateMonitorThread()))
    {
        assert(0);
    }
    pMiniThing->StartMonitorThread();

    Sleep(1000 * 1200);

    // Exit monitor thread and SQLite
    pMiniThing->StopMonitorThread();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
