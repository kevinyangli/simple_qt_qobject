#include "simple_qobject.h"


thread_local base::CEventLoop* base::CEventLoop::s_currentThreadEventLoop = nullptr;