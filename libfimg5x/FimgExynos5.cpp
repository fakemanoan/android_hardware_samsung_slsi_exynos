/*
**
** Copyright 2009 Samsung Electronics Co, Ltd.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
**
*/

#define LOG_NDEBUG 0
#define LOG_TAG "FimgExynos5"
#include <utils/Log.h>

#ifdef FIMG2D_USE_M2M1SHOT2
#include <linux/m2m1shot2.h>
#endif

#include "FimgExynos5.h"

#pragma clang diagnostic ignored "-Wunreachable-code-loop-increment"

extern pthread_mutex_t s_g2d_lock;

namespace android
{
unsigned   FimgV4x::m_curFimgV4xIndex = 0;
int        FimgV4x::m_numOfInstance    = 0;
FimgApi *  FimgV4x::m_ptrFimgApiList[NUMBER_FIMG_LIST] = {NULL, };

//---------------------------------------------------------------------------//

FimgV4x::FimgV4x()
         : m_g2dFd(0),
           m_g2dVirtAddr(NULL),
           m_g2dSize(0),
           m_g2dSrcVirtAddr(NULL),
           m_g2dSrcSize(0),
           m_g2dDstVirtAddr(NULL),
           m_g2dDstSize(0)
{
    memset(&(m_g2dPoll), 0, sizeof(struct pollfd));
}

FimgV4x::~FimgV4x()
{
}

FimgApi *FimgV4x::CreateInstance()
{
    FimgApi *ptrFimg = NULL;

    for(int i = m_curFimgV4xIndex; i < NUMBER_FIMG_LIST; i++) {
        if (m_ptrFimgApiList[i] == NULL)
            m_ptrFimgApiList[i] = new FimgV4x;

        if (m_ptrFimgApiList[i]->FlagCreate() == false) {
            if (m_ptrFimgApiList[i]->Create() == false) {
                PRINT("%s::Create(%d) fail\n", __func__, i);
                goto CreateInstance_End;
            }
            else
                m_numOfInstance++;
        }

        if (i < NUMBER_FIMG_LIST - 1)
            m_curFimgV4xIndex = i + 1;
        else
            m_curFimgV4xIndex = 0;

        ptrFimg = m_ptrFimgApiList[i];
        goto CreateInstance_End;
    }

CreateInstance_End :

    return ptrFimg;
}

void FimgV4x::DestroyInstance(FimgApi * ptrFimgApi)
{
    pthread_mutex_lock(&s_g2d_lock);

    for(int i = 0; i < NUMBER_FIMG_LIST; i++) {
        if (m_ptrFimgApiList[i] != NULL && m_ptrFimgApiList[i] == ptrFimgApi) {
            if (m_ptrFimgApiList[i]->FlagCreate() == true && m_ptrFimgApiList[i]->Destroy() == false) {
                PRINT("%s::Destroy() fail\n", __func__);
            } else {
                FimgV4x * tempFimgV4x = (FimgV4x *)m_ptrFimgApiList[i];
                delete tempFimgV4x;
                m_ptrFimgApiList[i] = NULL;

                m_numOfInstance--;
            }

            break;
        }
    }
    pthread_mutex_unlock(&s_g2d_lock);
}

void FimgV4x::DestroyAllInstance(void)
{
    pthread_mutex_lock(&s_g2d_lock);

    for (int i = 0; i < NUMBER_FIMG_LIST; i++) {
        if (m_ptrFimgApiList[i] != NULL) {
            if (m_ptrFimgApiList[i]->FlagCreate() == true
               && m_ptrFimgApiList[i]->Destroy() == false) {
                    PRINT("%s::Destroy() fail\n", __func__);
            } else {
                FimgV4x * tempFimgV4x = (FimgV4x *)m_ptrFimgApiList[i];
                delete tempFimgV4x;
                m_ptrFimgApiList[i] = NULL;
            }
        }
    }
    pthread_mutex_unlock(&s_g2d_lock);
}

bool FimgV4x::t_Create(void)
{
    bool ret = true;

    if (m_CreateG2D() == false) {
        PRINT("%s::m_CreateG2D() fail \n", __func__);

        if (m_DestroyG2D() == false)
            PRINT("%s::m_DestroyG2D() fail \n", __func__);

        ret = false;
    }

    return ret;
}

bool FimgV4x::t_Destroy(void)
{
    bool ret = true;

    if (m_DestroyG2D() == false) {
        PRINT("%s::m_DestroyG2D() fail \n", __func__);
        ret = false;
    }

    return ret;
}
#ifdef FIMG2D_USE_M2M1SHOT2
bool FimgV4x::t_Stretch_v5(struct m2m1shot2 *cmd)
{
#ifdef CHECK_FIMGV4x_PERFORMANCE
#define NUM_OF_STEP (10)
    StopWatch   stopWatch("CHECK_FIMGV4x_PERFORMANCE");
    const char *stopWatchName[NUM_OF_STEP];
    nsecs_t     stopWatchTime[NUM_OF_STEP];
    int         stopWatchIndex = 0;
#endif // CHECK_FIMGV4x_PERFORMANCE

    if (m_DoG2D_v5(cmd) == false)
        goto STRETCH_FAIL;

#ifdef G2D_NONE_BLOCKING_MODE
    if (m_PollG2D(&m_g2dPoll) == false) {
        PRINT("%s::m_PollG2D() fail\n", __func__);
        goto STRETCH_FAIL;
    }
#endif

#ifdef CHECK_FIMGV4x_PERFORMANCE
    m_PrintFimgV4xPerformance(src, dst, stopWatchIndex, stopWatchName, stopWatchTime);
#endif // CHECK_FIMGV4x_PERFORMANCE

    return true;

STRETCH_FAIL:
    return false;

}
#endif
bool FimgV4x::t_Stretch(struct fimg2d_blit *cmd)
{
#ifdef CHECK_FIMGV4x_PERFORMANCE
#define NUM_OF_STEP (10)
    StopWatch   stopWatch("CHECK_FIMGV4x_PERFORMANCE");
    const char *stopWatchName[NUM_OF_STEP];
    nsecs_t     stopWatchTime[NUM_OF_STEP];
    int         stopWatchIndex = 0;
#endif // CHECK_FIMGV4x_PERFORMANCE

    if (m_DoG2D(cmd) == false)
        goto STRETCH_FAIL;

#ifdef G2D_NONE_BLOCKING_MODE
    if (m_PollG2D(&m_g2dPoll) == false) {
        PRINT("%s::m_PollG2D() fail\n", __func__);
        goto STRETCH_FAIL;
    }
#endif

#ifdef CHECK_FIMGV4x_PERFORMANCE
    m_PrintFimgV4xPerformance(src, dst, stopWatchIndex, stopWatchName, stopWatchTime);
#endif // CHECK_FIMGV4x_PERFORMANCE

    return true;

STRETCH_FAIL:
    return false;

}

bool FimgV4x::t_Sync(void)
{
    if (m_PollG2D(&m_g2dPoll) == false) {
        PRINT("%s::m_PollG2D() fail\n", __func__);
        goto SYNC_FAIL;
    }
    return true;

SYNC_FAIL:
    return false;

}

bool FimgV4x::t_Lock(void)
{
    return m_lock.lock();
}

bool FimgV4x::t_UnLock(void)
{
    m_lock.unlock();
    return true;
}

bool FimgV4x::m_CreateG2D(void)
{
    int val = 0;

    if (m_g2dFd != 0) {
        PRINT("%s::m_g2dFd(%d) is not 0 fail\n", __func__, m_g2dFd);
        return false;
    }

#ifdef G2D_NONE_BLOCKING_MODE
    m_g2dFd = open(SEC_G2D_DEV_NAME, O_RDWR | O_NONBLOCK);
#else
    m_g2dFd = open(SEC_G2D_DEV_NAME, O_RDWR);
#endif
    if (m_g2dFd < 0) {
        PRINT("%s::open(%s) fail(%s)\n", __func__, SEC_G2D_DEV_NAME, strerror(errno));
        m_g2dFd = 0;
        return false;
    }
    val = fcntl(m_g2dFd, F_GETFD, 0);
    if (val < 0) {
        PRINT("%s::GETFD(%s) fail\n", __func__, SEC_G2D_DEV_NAME);
        close(m_g2dFd);
        m_g2dFd = 0;
        return false;
    }
    val = fcntl(m_g2dFd, F_SETFD, val | FD_CLOEXEC);
    if (val < 0) {
        PRINT("%s::SETFD(%s) fail\n", __func__, SEC_G2D_DEV_NAME);
        close(m_g2dFd);
        m_g2dFd = 0;
        return false;
    }

    memset(&m_g2dPoll, 0, sizeof(m_g2dPoll));
    m_g2dPoll.fd     = m_g2dFd;
    m_g2dPoll.events = POLLOUT | POLLERR;

    return true;
}

bool FimgV4x::m_DestroyG2D(void)
{
    if (m_g2dVirtAddr != NULL) {
        munmap(m_g2dVirtAddr, m_g2dSize);
        m_g2dVirtAddr = NULL;
        m_g2dSize = 0;
    }

    if (0 < m_g2dFd)
        close(m_g2dFd);

    m_g2dFd = 0;

    return true;
}
#ifdef FIMG2D_USE_M2M1SHOT2
bool FimgV4x::m_DoG2D_v5(struct m2m1shot2 *cmd)
{
    if (ioctl(m_g2dFd, M2M1SHOT2_IOC_PROCESS, cmd) < 0)
        return false;
    /* support the error handling */
    if (cmd->flags & M2M1SHOT2_FLAG_ERROR) {
        ALOGE("%s: hardware operation failed", __func__);
        return false;
    }
    return true;
}
#endif
bool FimgV4x::m_DoG2D(struct fimg2d_blit *cmd)
{
    if (ioctl(m_g2dFd, FIMG2D_BITBLT_BLIT, cmd) < 0)
        return false;

    /* wait to complte the blit */
    ioctl(m_g2dFd, FIMG2D_BITBLT_SYNC);

    return true;
}

inline bool FimgV4x::m_PollG2D(struct pollfd * events)
{
#define G2D_POLL_TIME (1000)

    int ret;

    ret = poll(events, 1, G2D_POLL_TIME);

    if (ret < 0) {
        PRINT("%s::poll fail \n", __func__);
        return false;
    }
    else if (ret == 0) {
        PRINT("%s::No data in %d milli secs..\n", __func__, G2D_POLL_TIME);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------//
// extern function
//---------------------------------------------------------------------------//
extern "C" FimgApi * createFimgApi()
{
    if (fimgApiAutoFreeThread == 0)
        fimgApiAutoFreeThread = new FimgApiAutoFreeThread();
    else
        fimgApiAutoFreeThread->SetOneMoreSleep();

    return FimgV4x::CreateInstance();
}

extern "C" void destroyFimgApi(__attribute__((__unused__)) FimgApi * ptrFimgApi)
{
    // Dont' call DestroyInstance.
}

extern "C" bool checkScaleFimgApi(Fimg *fimg)
{
    unsigned int i;
    for (i = 0; i < sizeof(compare_size) / sizeof(compare_size[0]); i++) {
        if ((fimg->srcW == compare_size[i][0]) && (fimg->srcH == compare_size[i][1]) &&
                (fimg->dstW == compare_size[i][2]) && (fimg->dstH == compare_size[i][3]))
            return true;
    }
    return false;
}

}; // namespace android
