/*
 * File         : es_timer.h
 * Module       : Timer Class
 * System       : Generic System
 * Project      : Foundation Classes Construction
 * Start Date   : 15th April 1994
 * Version      : 01.00
 * Version Date : 15-10-12
 * Designer     : Jack Lee
 * Programmer   : Jack Lee
 * Copyright    : Jack Lee 1993-2013 All Rights Reserved
 * Descriptions : Header file.
 * Remarks      : Portable version which supports WIN32 and Linux Timers
 *                For Linux need to add '-lrt' to linker to link with the RT timer lib
 * References   :
 *
 */

#ifndef     __ES_TIMER_H
#define     __ES_TIMER_H

///////////////////////////////////
////////// Include Files //////////
///////////////////////////////////

#ifdef WIN32 // Windows
#  include <windows.h>
#  include <sys/types.h>
#  include <sys/timeb.h>
#  include <dos.h>
#  include <time.h>
#else // Assume Linux
#  include <sys/types.h>
//#  include <sys/timeb.h>
#  include <time.h>
#endif

///////////////////////////////////
////// Constants Definition ///////
///////////////////////////////////

//#define INTEGER_64_AVAILABLE

///////////////////////////////////
/// Global Variables Definition ///
///////////////////////////////////

///////////////////////////////////
/// Class/Function Declarations ///
///////////////////////////////////


/* Class Name     : ES_FlashTimer
 * Descriptions   : An accurate time-keeping class, supporting stop-watch like
 *                  features.
 *
 *
 * Notes          : This class is OS dependent.
 * Remarks        : All functions are in-lined.
 *                  Available for MS-Windows 3.1 and Windows NT.
 * BUG            : In other OS, the slow standard C timer lib is used.
 */

#ifdef WIN32
class ES_FlashTimer
{
public:

   ES_FlashTimer()
   {
      pCounter = new LARGE_INTEGER;
      QueryPerformanceFrequency(pCounter);
#ifdef INTEGER_64_AVAILABLE
      NumPerUSec = *((__int64 *)(pCounter)) / 1000000;
      NumPerMSec = *((__int64 *)(pCounter)) / 1000;
#else
      NumPerUSec = C64tLD(*pCounter) / 1000000;
      NumPerMSec = C64tLD(*pCounter) / 1000;
#endif
      Start();
      StartX();
   }

   ~ES_FlashTimer()
   {
      delete pCounter;
   }

   long Start()
   {
      Refresh();
      StartMs = *pCounter;
#ifdef INTEGER_64_AVAILABLE
      return ((*(__int64 *)&StartMs)) / NumPerMSec;
#else
      return long(C64tLD(StartMs) / NumPerMSec);
#endif
   }

   long StartX()
   {
      Refresh();
      StartXMs  = *pCounter;
#ifdef INTEGER_64_AVAILABLE
      return ((*(__int64 *)&StartXMs)) / NumPerMSec;
#else
      return long(C64tLD(StartXMs) / NumPerMSec);
#endif
   }

   long Elapsed()
   {
      Refresh();
#ifdef INTEGER_64_AVAILABLE
      return (*((__int64 *)(pCounter)) - ((*(__int64 *)&StartMs)))/NumPerMSec;
#else
      return (C64tLD(*pCounter) - C64tLD(StartMs))/NumPerMSec;
#endif
   }

   long ElapseduSec()
   {
      Refresh();
#ifdef INTEGER_64_AVAILABLE
      return (*((__int64 *)(pCounter)) - ((*(__int64 *)&StartMs)))/NumPerUSec;
#else
      return (C64tLD(*pCounter) - C64tLD(StartMs))/NumPerUSec;
#endif
   }

   long ElapsedX()
   {
      Refresh();
#ifdef INTEGER_64_AVAILABLE
      return (*((__int64 *)(pCounter)) - ((*(__int64 *)&StartXMs)))/NumPerMSec;
#else
      return (C64tLD(*pCounter) - C64tLD(StartXMs))/NumPerMSec;
#endif
   }

   void Refresh()
   {
      QueryPerformanceCounter(pCounter);
   }

protected:

#ifdef INTEGER_64_AVAILABLE
   __int64        NumPerMSec,NumPerUSec;
#else  // Use long double instead.
   long double    NumPerMSec,NumPerUSec;
   long double    C64tLD(const LARGE_INTEGER &val)
   { return (long double) val.u.HighPart * 0xFFFFFFFF + val.u.LowPart; }
#endif

   LARGE_INTEGER  StartMs, StartXMs;
   LARGE_INTEGER  *pCounter;
};

#else // Linux

class ES_FlashTimer
{
public:

   ES_FlashTimer()
   {
	   StartMs = new struct timespec;
	   StartXMs = new struct timespec;
	   pResolution = new struct timespec;
	   pNow = new struct timespec;
	   QueryPerformanceFrequency();
	   Start();
	   StartX();
   }

   ~ES_FlashTimer()
   {
      delete StartMs;
      delete StartXMs;
      delete pResolution;
      delete pNow;
   }

   long Start()
   {
      Refresh();
      StartMs->tv_sec = pNow->tv_sec;
      StartMs->tv_nsec = pNow->tv_nsec;
      return 0;
   }

   long StartX()
   {
	   Refresh();
	   StartMs->tv_sec = pNow->tv_sec;
	   StartMs->tv_nsec = pNow->tv_nsec;
	   return 0;
   }

   long Elapsed()
   {
      Refresh();
      long retval = (pNow->tv_sec - StartMs->tv_sec)*1000;
      retval += (pNow->tv_nsec - StartMs->tv_nsec)/1000000;
      return retval;
   }

   long ElapseduSec()
   {
      Refresh();
      long retval = (pNow->tv_sec - StartMs->tv_sec)*1000000;
      retval += (pNow->tv_nsec - StartMs->tv_nsec)/1000;
      return retval;
   }

   long ElapsedX()
   {
      Refresh();
      long retval = (pNow->tv_sec - StartMs->tv_sec)*1000;
      retval += (pNow->tv_nsec - StartMs->tv_nsec)/1000000;
      return retval;
   }

   void Refresh()
   {
      clock_gettime(CLOCK_REALTIME, pNow);
   }

   void QueryPerformanceFrequency()
   {
	   clock_getres(CLOCK_REALTIME, pResolution);
   }

protected:

   struct timespec  *StartMs, *StartXMs;
   struct timespec  *pResolution, *pNow;
};

#endif


///////////////////////////////////
///// Inline Member Functions /////
///////////////////////////////////


///////////////////////////////////
////////// End of File ////////////
///////////////////////////////////

#endif
