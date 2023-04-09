// MiniThing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "MiniThing.h"
#include "Utility/Utility.h"

int main()
{
    MiniThing* pMiniThing = new MiniThing(L"F:", ".\\MiniThing.db");

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
