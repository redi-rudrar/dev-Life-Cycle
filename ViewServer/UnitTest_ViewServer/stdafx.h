// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <WinSock2.h>
#include <stdio.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here
#include "gmock/gmock.h"
#include "gtest/gtest.h"
using namespace ::testing;

#include "vsdefs.h"
#include "ViewServer.h"