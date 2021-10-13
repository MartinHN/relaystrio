#pragma once
#include "./lib/JPI/src/API.hpp"

#define SCHEDULE_TESTS 1

struct YSchedule {};
struct SchedulerAPI : public APIAndInstance<SchedulerAPI>, LeafNode {

  SchedulerAPI() : APIAndInstance<SchedulerAPI>(this) {}

  SchedulerAPI(SchedulerAPI &) = delete;

  bool setup() override { return true; }
  bool shouldBeOn() { return true; }
};
