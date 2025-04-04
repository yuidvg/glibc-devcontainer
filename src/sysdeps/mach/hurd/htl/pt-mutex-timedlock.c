/* pthread_mutex_timedlock.  Hurd version.
   Copyright (C) 2016-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library;  if not, see
   <https://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <pt-internal.h>
#include "pt-mutex.h"
#include <hurdlock.h>

int
__pthread_mutex_timedlock (pthread_mutex_t *mtxp, const struct timespec *tsp)
{
  struct __pthread *self;
  int ret, flags = mtxp->__flags & GSYNC_SHARED;

  switch (MTX_TYPE (mtxp))
    {
    case PT_MTX_NORMAL:
      ret = lll_abstimed_lock (mtxp->__lock, tsp, flags);
      break;

    case PT_MTX_RECURSIVE:
      self = _pthread_self ();
      if (mtx_owned_p (mtxp, self, flags))
	{
	  if (__glibc_unlikely (mtxp->__cnt + 1 == 0))
	    return EAGAIN;

	  ++mtxp->__cnt;
	  ret = 0;
	}
      else if ((ret = lll_abstimed_lock (mtxp->__lock, tsp, flags)) == 0)
	{
	  mtx_set_owner (mtxp, self, flags);
	  mtxp->__cnt = 1;
	}

      break;

    case PT_MTX_ERRORCHECK:
      self = _pthread_self ();
      if (mtx_owned_p (mtxp, self, flags))
	ret = EDEADLK;
      else if ((ret = lll_abstimed_lock (mtxp->__lock, tsp, flags)) == 0)
	mtx_set_owner (mtxp, self, flags);

      break;

    case PT_MTX_NORMAL | PTHREAD_MUTEX_ROBUST:
    case PT_MTX_RECURSIVE | PTHREAD_MUTEX_ROBUST:
    case PT_MTX_ERRORCHECK | PTHREAD_MUTEX_ROBUST:
      self = _pthread_self ();
      ROBUST_LOCK (self, mtxp, lll_robust_abstimed_lock, tsp, flags);
      break;

    default:
      ret = EINVAL;
      break;
    }

  return ret;
}
strong_alias (__pthread_mutex_timedlock, pthread_mutex_timedlock)
