#include "driver.h"

#include "driver/refresh.h"
#include "driver/symbols.h"

void InitDriverBridge(const char *selfDir) {
	RefreshDx12Impl(selfDir);
	LoadDx12Impl(selfDir);
}
