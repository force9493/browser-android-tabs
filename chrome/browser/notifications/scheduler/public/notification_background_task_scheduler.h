// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_SCHEDULER_PUBLIC_NOTIFICATION_BACKGROUND_TASK_SCHEDULER_H_
#define CHROME_BROWSER_NOTIFICATIONS_SCHEDULER_PUBLIC_NOTIFICATION_BACKGROUND_TASK_SCHEDULER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/notifications/scheduler/public/notification_scheduler_types.h"

namespace notifications {

// Interface to schedule a background task on platform OS to run the
// notification scheduler job.
class NotificationBackgroundTaskScheduler {
 public:
  // Interface used to handle background task events.
  class Handler {
   public:
    using TaskFinishedCallback = base::OnceCallback<void(bool)>;
    // Called when the background task is started.
    virtual void OnStartTask(SchedulerTaskTime task_time,
                             TaskFinishedCallback callback) = 0;

    // Called when the background task is stopped by the OS when it wants to
    // reallocate resources, our task is not finished yet in this case. The
    // handler implementation should explicitly decide whether the task should
    // be rescheduled and run later.
    virtual void OnStopTask(SchedulerTaskTime task_time) = 0;

   protected:
    Handler() = default;
    virtual ~Handler() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(Handler);
  };

  // Schedules a background task in a time window between |window_start| and
  // |window_end|. This will update the current background task. Only one
  // background task exists for notification scheduler. |scheduler_task_time|
  // tag is passed to background task go support arbitrary time background task.
  virtual void Schedule(notifications::SchedulerTaskTime scheduler_task_time,
                        base::TimeDelta window_start,
                        base::TimeDelta window_end) = 0;

  // Cancels the background task.
  virtual void Cancel() = 0;

  virtual ~NotificationBackgroundTaskScheduler() = default;

 protected:
  NotificationBackgroundTaskScheduler() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationBackgroundTaskScheduler);
};

}  // namespace notifications

#endif  // CHROME_BROWSER_NOTIFICATIONS_SCHEDULER_PUBLIC_NOTIFICATION_BACKGROUND_TASK_SCHEDULER_H_
