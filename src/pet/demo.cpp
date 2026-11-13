#include "pet/demo.h"
#include "pet/state.h"
#include "config.h"

struct DemoStep {
  State state;
  const char *tool;
};

static const DemoStep STEPS[] = {
  { THINKING, "" },
  { WORKING, "Bash" },
  { WORKING, "Edit" },
  { WORKING, "Read" },
  { WORKING, "Task" },
  { WORKING, "mcp_tool" },
  { ATTENTION, "" },
  { DONE, "" },
  { IDLE, "" },
};
static constexpr int STEP_COUNT = sizeof(STEPS) / sizeof(STEPS[0]);

void demoTick(uint32_t now) {
  static int current = -1;
  int step = (now / DEMO_STEP_MS) % STEP_COUNT;
  if (step != current) {
    current = step;
    petEvent(STEPS[step].state, STEPS[step].tool);
  }
}
