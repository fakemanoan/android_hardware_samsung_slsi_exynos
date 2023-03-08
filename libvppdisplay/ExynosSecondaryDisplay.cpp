#include "ExynosSecondaryDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#ifdef USES_TWO_DECON
//#define LOG_NDEBUG 0
#define LOG_TAG "hwcomposer"

ExynosSecondaryDisplay::ExynosSecondaryDisplay(struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_SECONDARY_DISPLAY, pdev)
{
    int refreshRate;
    this->mHwc = pdev;

    mDisplayFd = open("/dev/graphics/fb1", O_RDWR);
    if (mDisplayFd < 0) {
        ALOGE("failed to open secondary display framebuffer");
        return;
    }

    struct fb_var_screeninfo info;
    if (ioctl(mDisplayFd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("FBIOGET_VSCREENINFO ioctl failed: %s", strerror(errno));
        return;
    }

    if (info.reserved[0] == 0 && info.reserved[1] == 0) {
        /* save physical lcd width, height to reserved[] */
        info.reserved[0] = info.xres;
        info.reserved[1] = info.yres;

        if (ioctl(mDisplayFd, FBIOPUT_VSCREENINFO, &info) == -1) {
            ALOGE("FBIOPUT_VSCREENINFO ioctl failed: %s", strerror(errno));
            return;
        }
    }

    /* restore physical lcd width, height from reserved[] */
    int lcd_xres, lcd_yres;
    lcd_xres = info.reserved[0];
    lcd_yres = info.reserved[1];

    refreshRate = 1000000000000LLU /
        (
         uint64_t( info.upper_margin + info.lower_margin + lcd_yres )
         * ( info.left_margin  + info.right_margin + lcd_xres )
         * info.pixclock
        );

    if (refreshRate == 0) {
        ALOGW("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60;
    }

    mXres = lcd_xres;
    mYres = lcd_yres;
    mXdpi = 1000 * (lcd_xres * 25.4f) / info.width;
    mYdpi = 1000 * (lcd_yres * 25.4f) / info.height;
    mVsyncPeriod  = 1000000000 / refreshRate;

    ALOGD("Dual-decon 2nd display using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "width        = %d mm (%f dpi)\n"
          "height       = %d mm (%f dpi)\n"
          "refresh rate = %d Hz\n",
          mXres, mYres, info.width, mXdpi / 1000.0,
          info.height, mYdpi / 1000.0, refreshRate);

    //close(mDisplayFd);
    //mDisplayFd = -1;
}

ExynosSecondaryDisplay::~ExynosSecondaryDisplay()
{
    disable();
}

int ExynosSecondaryDisplay::enable()
{

    if (mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = true;

    return 0;
}

int ExynosSecondaryDisplay::disable()
{
    if (!mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = false;

    return 0;
}


void ExynosSecondaryDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
    //mInternalDMAs.add(IDMA_G0);
    ExynosDisplay::doPreProcessing(contents);
}

#else/*USES_TWO_DECON*/
#ifdef USES_SINGLE_DECON
#include "ExynosPrimaryDisplay.h"

#define LOG_TAG "hwcomposer"
class ExynosPrimaryDisplay;

ExynosSecondaryDisplay::ExynosSecondaryDisplay(struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_SECONDARY_DISPLAY, pdev)
{
    this->mHwc = pdev;
    mXres = pdev->primaryDisplay->mXres;
    mYres = pdev->primaryDisplay->mYres;
    mXdpi = pdev->primaryDisplay->mXdpi;
    mYdpi = pdev->primaryDisplay->mYres;
    mVsyncPeriod  = pdev->primaryDisplay->mVsyncPeriod;

    ALOGD("ExynosSecondaryDisplay using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "xdpi         = %f dpi\n"
          "ydpi         = %f dpi\n"
          "vsyncPeriod  = %d msec\n",
          mXres, mYres, mXdpi, mYdpi, mVsyncPeriod);
}

ExynosSecondaryDisplay::~ExynosSecondaryDisplay()
{
    disable();
}

int ExynosSecondaryDisplay::enable()
{
    if (mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = true;

    return 0;
}

int ExynosSecondaryDisplay::disable()
{
    if (!mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = false;

    return 0;
}

void ExynosSecondaryDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
    mInternalDMAs.add(IDMA_G0);
    ExynosDisplay::doPreProcessing(contents);
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left += (mXres/2);
        layer.displayFrame.right += (mXres/2);
    }
}

int ExynosSecondaryDisplay::prepare(hwc_display_contents_1_t *contents)
{
    ExynosDisplay::prepare(contents);
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left -= (mXres/2);
        layer.displayFrame.right -= (mXres/2);
    }
    return 0;
}

int ExynosSecondaryDisplay::set(hwc_display_contents_1_t *contents)
{
    int ret = 0;
    /* Change position */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left += (mXres/2);
        layer.displayFrame.right += (mXres/2);
    }

    ret = ExynosDisplay::set(contents);

    /* Restore position */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left -= (mXres/2);
        layer.displayFrame.right -= (mXres/2);
    }

    return ret;
}
#endif/*USES_SINGLE_DECON*/
#endif/*USES_TWO_DECON*/

