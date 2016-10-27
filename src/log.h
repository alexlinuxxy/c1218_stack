#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#ifdef __LOG__
#ifdef __DEBUG__
#define LOGI(...) do {\
	printf("[INFO] "__VA_ARGS__);\
} while (0)

#define LOGW(...) do {\
	printf("[WARN] "__VA_ARGS__);\
} while (0)

#define LOGE(...) do {\
	printf("[ERROR] "__VA_ARGS__);\
} while (0)

#define LOGD(...) do {\
	printf("[DEBUG] "__VA_ARGS__);\
} while (0)
#else
extern void LOGI(__VA_ARGS__);
extern void LOGW(__VA_ARGS__);
extern void LOGE(__VA_ARGS__);
extern void LOGD(__VA_ARGS__);
#endif
#endif

#endif // LOG_H_
