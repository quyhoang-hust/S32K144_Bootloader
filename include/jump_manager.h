#ifndef JUMP_MANAGER_H
#define JUMP_MANAGER_H

#include <stdbool.h>
#include "compiler_abstraction.h"

bool JumpManager_CanJump(void);

NO_RETURN void JumpManager_JumpToApplication(void);

bool JumpManager_TryJumpToApplication(void);

#endif
