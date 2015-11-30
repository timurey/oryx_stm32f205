//
// This file is part of the µOS++ III distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// Do not include on semihosting and when freestanding
#if !defined(OS_USE_SEMIHOSTING) && !(__STDC_HOSTED__ == 0)

// ----------------------------------------------------------------------------

#include <errno.h>
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "diag/Trace.h"
extern USBD_HandleTypeDef  hUsbDeviceFS;
// ----------------------------------------------------------------------------

// When using retargetted configurations, the standard write() system call,
// after a long way inside newlib, finally calls this implementation function.

// Based on the file descriptor, it can send arrays of characters to
// different physical devices.

// Currently only the output and error file descriptors are tested,
// and the characters are forwarded to the trace device, mainly
// for demonstration purposes. Adjust it for your specific needs.

// For freestanding applications this file is not used and can be safely
// ignored.

ssize_t
_write (int fd, const char* buf, size_t nbyte);

ssize_t
_write (int fd __attribute__((unused)), const char* buf __attribute__((unused)),
	size_t nbyte __attribute__((unused)))
{
#if defined(TRACE)
  // STDOUT and STDERR are routed to the trace device
  if (fd == 1 || fd == 2)
    {
      return trace_write (buf, nbyte);
    }
#else // TRACE
  static int failed;
    if(fd == 1) {
      if(hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
        return 0;
      if(failed) {
        if(CDC_Transmit_FS(buf, nbyte) == USBD_OK) {
          failed = 0;
          return nbyte;
        }
        return 0;
      }
      uint32_t timeout = HAL_GetTick() + 500; // 0.5 second timeout
      while(HAL_GetTick() < timeout) {
        if(hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
          return 0;
        int result = CDC_Transmit_FS(buf, nbyte);
        if(result == USBD_OK)
          return nbyte;
        if(result != USBD_BUSY)
          return -1;
        // save some power!
//        __WFI();
      }
      failed = 1;
      return 0;
    }
#endif
  errno = ENOSYS;
  return -1;
}

// ----------------------------------------------------------------------------

#endif // !defined(OS_USE_SEMIHOSTING) && !(__STDC_HOSTED__ == 0)
