//
//  kern_weg.cpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#include "kern_wred.hpp"

#include <IOKit/graphics/IOFramebuffer.h>

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>

// This is a hack to let us access protected properties.
struct FramebufferViewer : public IOFramebuffer {
    static IOMemoryMap *&getVramMap(IOFramebuffer *fb) {
        return static_cast<FramebufferViewer *>(fb)->fVramMap;
    }
};

static const char *pathIOGraphics[] = {
    "/System/Library/Extensions/IOGraphicsFamily.kext/IOGraphicsFamily"};
static const char *pathAGDPolicy[] = {
    "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
    "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy"};
static const char *pathBacklight[] = {
    "/System/Library/Extensions/AppleBacklight.kext/Contents/MacOS/"
    "AppleBacklight"};
static const char *pathMCCSControl[] = {
    "/System/Library/Extensions/AppleMCCSControl.kext/Contents/MacOS/"
    "AppleMCCSControl"};

static KernelPatcher::KextInfo kextIOGraphics{
    "com.apple.iokit.IOGraphicsFamily",
    pathIOGraphics,
    arrsize(pathIOGraphics),
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextAGDPolicy{
    "com.apple.driver.AppleGraphicsDevicePolicy",
    pathAGDPolicy,
    arrsize(pathAGDPolicy),
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
// Note: initially marked as reloadable, but I doubt it needs to be.
static KernelPatcher::KextInfo kextBacklight{
    "com.apple.driver.AppleBacklight",
    pathBacklight,
    arrsize(pathBacklight),
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextMCCSControl{
    "com.apple.driver.AppleMCCSControl",
    pathMCCSControl,
    arrsize(pathMCCSControl),
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

WRed::ApplePanelData WRed::appleBacklightData[]{
    {"F14Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94,
      0x00, 0xBE, 0x00, 0xFA, 0x01, 0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F,
      0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06, 0x0E, 0x07, 0x10}},
    {"F15Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x36, 0x00, 0x54, 0x00, 0x7D, 0x00, 0xB2,
      0x00, 0xF5, 0x01, 0x49, 0x01, 0xB1, 0x02, 0x2B, 0x02, 0xB8, 0x03, 0x59,
      0x04, 0x13, 0x04, 0xEC, 0x05, 0xF3, 0x07, 0x34, 0x08, 0xAF, 0x0A, 0xD9}},
    {"F16Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x18, 0x00, 0x27, 0x00, 0x3A, 0x00, 0x52,
      0x00, 0x71, 0x00, 0x96, 0x00, 0xC4, 0x00, 0xFC, 0x01, 0x40, 0x01, 0x93,
      0x01, 0xF6, 0x02, 0x6E, 0x02, 0xFE, 0x03, 0xAA, 0x04, 0x78, 0x05, 0x6C}},
    {"F17Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x34, 0x00, 0x4F, 0x00, 0x71,
      0x00, 0x9B, 0x00, 0xCF, 0x01, 0x0E, 0x01, 0x5D, 0x01, 0xBB, 0x02, 0x2F,
      0x02, 0xB9, 0x03, 0x60, 0x04, 0x29, 0x05, 0x1E, 0x06, 0x44, 0x07, 0xA1}},
    {"F18Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x00, 0x53, 0x00, 0x8C, 0x00, 0xD5, 0x01, 0x31,
      0x01, 0xA2, 0x02, 0x2E, 0x02, 0xD8, 0x03, 0xAE, 0x04, 0xAC, 0x05, 0xE5,
      0x07, 0x59, 0x09, 0x1C, 0x0B, 0x3B, 0x0D, 0xD0, 0x10, 0xEA, 0x14, 0x99}},
    {"F19Txxxx",
     {0x00, 0x11, 0x00, 0x00, 0x02, 0x8F, 0x03, 0x53, 0x04, 0x5A, 0x05, 0xA1,
      0x07, 0xAE, 0x0A, 0x3D, 0x0E, 0x14, 0x13, 0x74, 0x1A, 0x5E, 0x24, 0x18,
      0x31, 0xA9, 0x44, 0x59, 0x5E, 0x76, 0x83, 0x11, 0xB6, 0xC7, 0xFF, 0x7B}},
    {"F24Txxxx",
     {0x00, 0x11, 0x00, 0x01, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94,
      0x00, 0xBE, 0x00, 0xFA, 0x01, 0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F,
      0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06, 0x0E, 0x07, 0x10}}};

WRed *WRed::callbackWEG;

void WRed::init() {
    callbackWEG = this;

    // Background init fix is only necessary on 10.10 and newer.
    // Former boot-arg name is igfxrst.
    if (getKernelVersion() >= KernelVersion::Yosemite) {
        PE_parse_boot_argn("gfxrst", &resetFramebuffer,
                           sizeof(resetFramebuffer));
        if (resetFramebuffer >= FB_TOTAL) {
            SYSLOG("wred",
                   "invalid igfxrset value %d, falling back to autodetect",
                   resetFramebuffer);
            resetFramebuffer = FB_DETECT;
        }
    } else {
        resetFramebuffer = FB_NONE;
    }

    // Black screen fix is needed everywhere, but the form depends on the
    // boot-arg. Former boot-arg name is ngfxpatch.
    char agdp[128];
    if (PE_parse_boot_argn("agdpmod", agdp, sizeof(agdp)))
        processGraphicsPolicyStr(agdp);

    // Callback setup is only done here for compatibility.
    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) {
            static_cast<WRed *>(user)->processKernel(patcher);
        },
        this);

    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t index,
           mach_vm_address_t address, size_t size) {
            static_cast<WRed *>(user)->processKext(patcher, index, address,
                                                   size);
        },
        this);

    // Perform a background fix.
    if (resetFramebuffer != FB_NONE) lilu.onKextLoadForce(&kextIOGraphics);

    // Perform a black screen fix.
    if (graphicsDisplayPolicyMod != AGDP_NONE_SET)
        lilu.onKextLoad(&kextAGDPolicy);

    // Disable backlight patches if asked specifically.
    PE_parse_boot_argn("applbkl", &appleBacklightPatch,
                       sizeof(appleBacklightPatch));
    if (appleBacklightPatch != APPLBKL_OFF) {
        lilu.onKextLoad(&kextBacklight);
    }
    if (appleBacklightPatch == APPLBKL_NAVI10) {
        lilu.onKextLoadForce(&kextMCCSControl);
    }

    rad.init();
}

void WRed::deinit() { rad.deinit(); }

void WRed::processKernel(KernelPatcher &patcher) {
    // Correct GPU properties
    auto devInfo = DeviceInfo::create();
    if (devInfo) {
        devInfo->processSwitchOff();

        if (graphicsDisplayPolicyMod == AGDP_DETECT) { /* Default detect only */
            auto getAgpdMod = [this](IORegistryEntry *device) {
                auto prop = device->getProperty("agdpmod");
                if (prop) {
                    DBGLOG("wred", "found agdpmod in external GPU %s",
                           safeString(device->getName()));
                    const char *agdp = nullptr;
                    auto propStr = OSDynamicCast(OSString, prop);
                    auto propData = OSDynamicCast(OSData, prop);
                    if (propStr) {
                        agdp = propStr->getCStringNoCopy();
                    } else if (propData && propData->getLength() > 0) {
                        agdp = static_cast<const char *>(
                            propData->getBytesNoCopy());
                        if (agdp && agdp[propData->getLength() - 1] != '\0') {
                            DBGLOG("wred",
                                   "agdpmod config is not null terminated");
                            agdp = nullptr;
                        }
                    }
                    if (agdp) {
                        processGraphicsPolicyStr(agdp);
                        return true;
                    }
                }

                return false;
            };

            size_t extNum = devInfo->videoExternal.size();
            for (size_t i = 0; i < extNum; i++) {
                if (getAgpdMod(devInfo->videoExternal[i].video)) break;
            }
            if (devInfo->videoBuiltin != nullptr &&
                graphicsDisplayPolicyMod ==
                    AGDP_DETECT) /* Default detect only */
                getAgpdMod(devInfo->videoBuiltin);
        }

        // Do not inject properties unless non-Apple
        size_t extNum = devInfo->videoExternal.size();
        if (devInfo->firmwareVendor != DeviceInfo::FirmwareVendor::Apple) {
            DBGLOG("wred", "non-apple-fw proceeding with devprops %d",
                   graphicsDisplayPolicyMod);

            if (appleBacklightPatch == APPLBKL_DETECT &&
                devInfo->videoBuiltin != nullptr)
                WIOKit::getOSDataValue(devInfo->videoBuiltin, "applbkl",
                                       appleBacklightPatch);

            if (appleBacklightCustomName == nullptr &&
                devInfo->videoBuiltin != nullptr) {
                appleBacklightCustomName = OSDynamicCast(
                    OSData, devInfo->videoBuiltin->getProperty("applbkl-name"));
                appleBacklightCustomData = OSDynamicCast(
                    OSData, devInfo->videoBuiltin->getProperty("applbkl-data"));
                if (appleBacklightCustomName == nullptr ||
                    appleBacklightCustomData == nullptr)
                    appleBacklightCustomName = appleBacklightCustomData =
                        nullptr;
            }

            for (size_t i = 0; i < extNum; i++) {
                auto &v = devInfo->videoExternal[i];
                processExternalProperties(v.video, devInfo, v.vendor);

                // Assume that AMD GPU is the boot display.
                if (v.vendor == WIOKit::VendorID::ATIAMD &&
                    resetFramebuffer == FB_DETECT)
                    resetFramebuffer = FB_ZEROFILL;

                if (appleBacklightPatch == APPLBKL_DETECT)
                    WIOKit::getOSDataValue(v.video, "applbkl",
                                           appleBacklightPatch);

                if (appleBacklightCustomName == nullptr) {
                    appleBacklightCustomName = OSDynamicCast(
                        OSData, v.video->getProperty("applbkl-name"));
                    appleBacklightCustomData = OSDynamicCast(
                        OSData, v.video->getProperty("applbkl-data"));
                    if (appleBacklightCustomName == nullptr ||
                        appleBacklightCustomData == nullptr)
                        appleBacklightCustomName = appleBacklightCustomData =
                            nullptr;
                }
            }

            // Note, disabled Optimus will make videoExternal 0, so this case
            // checks for active IGPU only.
            DBGLOG("wred", "resulting applbkl value is %d",
                   appleBacklightPatch);
            if (appleBacklightPatch == APPLBKL_OFF ||
                (appleBacklightPatch == APPLBKL_DETECT &&
                 (devInfo->videoBuiltin == nullptr || extNum > 0))) {
                // Either a builtin IGPU is not available, or some external GPU
                // is available.
                kextBacklight.switchOff();
            }

            if ((graphicsDisplayPolicyMod & AGDP_DETECT) &&
                isGraphicsPolicyModRequired(devInfo))
                graphicsDisplayPolicyMod =
                    AGDP_VIT9696 | AGDP_PIKERA | AGDP_SET;
        } else {
            if (appleBacklightPatch != APPLBKL_ON) {
                // Do not patch AppleBacklight on Apple HW, unless forced.
                kextBacklight.switchOff();
            }

            // Support legacy -wegtree argument.
            bool rebuidTree = checkKernelArgument("-wegtree");

            // Support device properties.
            if (!rebuidTree && devInfo->videoBuiltin)
                rebuidTree = devInfo->videoBuiltin->getProperty(
                                 "rebuild-device-tree") != nullptr;

            for (size_t i = 0; !rebuidTree && i < extNum; i++)
                rebuidTree = devInfo->videoExternal[i].video->getProperty(
                                 "rebuild-device-tree") != nullptr;

            // Override with modern wegtree argument.
            int tree;
            if (PE_parse_boot_argn("wegtree", &tree, sizeof(tree)))
                rebuidTree = tree != 0;

            if (rebuidTree) {
                DBGLOG("wred", "apple-fw proceeding with devprops by request");

                for (size_t i = 0; i < extNum; i++) {
                    auto &v = devInfo->videoExternal[i];
                    processExternalProperties(v.video, devInfo, v.vendor);
                }
            }
        }

        rad.processKernel(patcher, devInfo);

        DeviceInfo::deleter(devInfo);
    }

    // Disable mods that did not find a way to function.
    if (resetFramebuffer == FB_DETECT) {
        resetFramebuffer = FB_NONE;
        kextIOGraphics.switchOff();
    }

    if ((graphicsDisplayPolicyMod & AGDP_DETECT) ||
        graphicsDisplayPolicyMod == AGDP_NONE_SET) {
        graphicsDisplayPolicyMod = AGDP_NONE_SET;
        kextAGDPolicy.switchOff();
    }

    // We need to load vinfo for cleanup and copy.
    if (resetFramebuffer == FB_COPY || resetFramebuffer == FB_ZEROFILL) {
        auto info = reinterpret_cast<vc_info *>(
            patcher.solveSymbol(KernelPatcher::KernelID, "_vinfo"));
        if (info) {
            consoleVinfo = *info;
            DBGLOG("wred", "vinfo 1: %u:%u %u:%u:%u", consoleVinfo.v_height,
                   consoleVinfo.v_width, consoleVinfo.v_depth,
                   consoleVinfo.v_rowbytes, consoleVinfo.v_type);
            DBGLOG("wred", "vinfo 2: %s %u:%u %u:%u:%u", consoleVinfo.v_name,
                   consoleVinfo.v_rows, consoleVinfo.v_columns,
                   consoleVinfo.v_rowscanbytes, consoleVinfo.v_scale,
                   consoleVinfo.v_rotate);
            gotConsoleVinfo = true;
        } else {
            SYSLOG("wred", "failed to obtain vcinfo");
            patcher.clearError();
        }
    }
}

size_t WRed::wrapFunctionReturnZero() { return 0; }

void WRed::processKext(KernelPatcher &patcher, size_t index,
                       mach_vm_address_t address, size_t size) {
    if (kextIOGraphics.loadIndex == index) {
        gIOFBVerboseBootPtr = patcher.solveSymbol<uint8_t *>(
            index, "__ZL16gIOFBVerboseBoot", address, size);
        if (gIOFBVerboseBootPtr) {
            KernelPatcher::RouteRequest request("__ZN13IOFramebuffer6initFBEv",
                                                wrapFramebufferInit,
                                                orgFramebufferInit);
            patcher.routeMultiple(index, &request, 1, address, size);
        } else {
            SYSLOG("wred", "failed to resolve gIOFBVerboseBoot");
            patcher.clearError();
        }
        return;
    }

    if (kextMCCSControl.loadIndex == index) {
        KernelPatcher::RouteRequest request[] = {
            {"__ZN25AppleMCCSControlGibraltar5probeEP9IOServicePi",
             wrapFunctionReturnZero},
            {"__ZN21AppleMCCSControlCello5probeEP9IOServicePi",
             wrapFunctionReturnZero},
        };
        patcher.routeMultiple(index, request, address, size);
        return;
    }

    if (kextAGDPolicy.loadIndex == index) {
        processGraphicsPolicyMods(patcher, address, size);
        return;
    }

    if (kextBacklight.loadIndex == index) {
        KernelPatcher::RouteRequest request(
            "__ZN15AppleIntelPanel10setDisplayEP9IODisplay",
            wrapApplePanelSetDisplay, orgApplePanelSetDisplay);
        if (patcher.routeMultiple(kextBacklight.loadIndex, &request, 1, address,
                                  size)) {
            const uint8_t find[] = {"F%uT%04x"};
            const uint8_t replace[] = {"F%uTxxxx"};
            KernelPatcher::LookupPatch patch = {&kextBacklight, find, replace,
                                                sizeof(find), 1};
            DBGLOG("wred", "applying backlight patch");
            patcher.applyLookupPatch(&patch);
        }
    }

    if (rad.processKext(patcher, index, address, size)) return;
}

void WRed::processExternalProperties(IORegistryEntry *device, DeviceInfo *info,
                                     uint32_t vendor) {
    auto name = device->getName();

    // It is unclear how to properly name the GPUs, and supposedly it does not
    // really matter. However, we will try to at least name them in a unique
    // manner (GFX0, GFX1, ...)
    if (device->getProperty("preserve-names") == nullptr &&
        currentExternalGfxIndex <= MaxExternalGfxIndex &&
        (!name || strncmp(name, "GFX", strlen("GFX")) != 0)) {
        char name[16];
        snprintf(name, sizeof(name), "GFX%u", currentExternalGfxIndex++);
        WIOKit::renameDevice(device, name);
    }

    // AAPL,slot-name is used to distinguish GPU slots in Mac Pro.
    // NVIDIA Web Drivers have a preference panel, where they read this value
    // and allow up to 4 GPUs. Each NVIDIA GPU is then displayed on the ECC tab.
    // We permit more slots, since 4 is an artificial restriction. iMac on the
    // other side has only one GPU and is not expected to have multiple slots.
    // Here we pass AAPL,slot-name if the GPU is NVIDIA or we have more than one
    // GPU.
    bool wantSlot =
        info->videoExternal.size() > 1 || vendor == WIOKit::VendorID::NVIDIA;
    if (wantSlot && currentExternalSlotIndex <= MaxExternalSlotIndex &&
        !device->getProperty("AAPL,slot-name")) {
        char name[16];
        snprintf(name, sizeof(name), "Slot-%u", currentExternalSlotIndex++);
        device->setProperty("AAPL,slot-name", name, sizeof("Slot-1"));
    }

    // Set the autodetected AMD GPU name here, it will later be handled by RAD
    // to not get overridden. This is not necessary for NVIDIA, as their drivers
    // properly detect the name.
    if (vendor == WIOKit::VendorID::ATIAMD && !device->getProperty("model")) {
        uint32_t dev, rev, subven, sub;
        if (WIOKit::getOSDataValue(device, "device-id", dev) &&
            WIOKit::getOSDataValue(device, "revision-id", rev) &&
            WIOKit::getOSDataValue(device, "subsystem-vendor-id", subven) &&
            WIOKit::getOSDataValue(device, "subsystem-id", sub)) {
            auto model = getRadeonModel(dev, rev, subven, sub);
            if (model) {
                device->setProperty("model", const_cast<char *>(model),
                                    static_cast<unsigned>(strlen(model) + 1));
            }
        }
    }

    // Ensure built-in.
    if (!device->getProperty("built-in")) {
        DBGLOG("wred", "fixing built-in");
        uint8_t builtBytes[]{0x00};
        device->setProperty("built-in", builtBytes, sizeof(builtBytes));
    } else {
        DBGLOG("wred", "found existing built-in");
    }
}

void WRed::processGraphicsPolicyStr(const char *agdp) {
    DBGLOG("wred", "agdpmod using config %s", agdp);
    if (strstr(agdp, "detect")) {
        graphicsDisplayPolicyMod = AGDP_DETECT_SET;
    } else if (strstr(agdp, "ignore")) {
        graphicsDisplayPolicyMod = AGDP_NONE_SET;
    } else {
        graphicsDisplayPolicyMod = AGDP_NONE_SET;
        if (strstr(agdp, "vit9696")) graphicsDisplayPolicyMod |= AGDP_VIT9696;
        if (strstr(agdp, "pikera")) graphicsDisplayPolicyMod |= AGDP_PIKERA;
        if (strstr(agdp, "cfgmap")) graphicsDisplayPolicyMod |= AGDP_CFGMAP;
    }
}

void WRed::processGraphicsPolicyMods(KernelPatcher &patcher,
                                     mach_vm_address_t address, size_t size) {
    if (graphicsDisplayPolicyMod & AGDP_VIT9696) {
        uint8_t find[] = {0xBA, 0x05, 0x00, 0x00, 0x00};
        uint8_t replace[] = {0xBA, 0x00, 0x00, 0x00, 0x00};
        KernelPatcher::LookupPatch patch{&kextAGDPolicy, find, replace,
                                         sizeof(find), 1};

        patcher.applyLookupPatch(&patch);
        if (patcher.getError() != KernelPatcher::Error::NoError) {
            SYSLOG("wred", "failed to apply agdp vit9696's patch %d",
                   patcher.getError());
            patcher.clearError();
        }
    }

    if (graphicsDisplayPolicyMod & AGDP_PIKERA) {
        KernelPatcher::LookupPatch patch{
            &kextAGDPolicy, reinterpret_cast<const uint8_t *>("board-id"),
            reinterpret_cast<const uint8_t *>("board-ix"), sizeof("board-id"),
            1};

        patcher.applyLookupPatch(&patch);
        if (patcher.getError() != KernelPatcher::Error::NoError) {
            SYSLOG("wred", "failed to apply agdp Piker-Alpha's patch %d",
                   patcher.getError());
            patcher.clearError();
        }
    }

    if (graphicsDisplayPolicyMod & AGDP_CFGMAP) {
        // Does not function in 10.13.x, as the symbols have been stripped.
        // Abort on usage on 10.14 or newer.
        if (getKernelVersion() >= KernelVersion::Mojave)
            PANIC(
                "wred",
                "adgpmod=cfgmap has no effect on 10.13.4, use agdpmod=ignore");
        KernelPatcher::RouteRequest request(
            "__ZN25AppleGraphicsDevicePolicy5startEP9IOService",
            wrapGraphicsPolicyStart, orgGraphicsPolicyStart);
        patcher.routeMultiple(kextAGDPolicy.loadIndex, &request, 1, address,
                              size);
    }
}

bool WRed::isGraphicsPolicyModRequired(DeviceInfo *info) {
    DBGLOG("wred", "detecting policy");
    // Graphics policy patches are only applicable to discrete GPUs.
    if (info->videoExternal.size() == 0) {
        DBGLOG("wred", "no external gpus");
        return false;
    }

    // Graphics policy patches do harm on Apple MacBooks, see:
    // https://github.com/acidanthera/bugtracker/issues/260
    if (info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple) {
        DBGLOG("wred", "apple firmware");
        return false;
    }

    // We do not need AGDC patches on compatible devices.
    auto boardId = BaseDeviceInfo::get().boardIdentifier;
    DBGLOG("wred", "board is %s", boardId);
    const char *compatibleBoards[]{
        "Mac-00BE6ED71E35EB86",  // iMac13,1
        "Mac-27ADBB7B4CEE8E61",  // iMac14,2
        "Mac-4B7AC7E43945597E",  // MacBookPro9,1
        "Mac-77EB7D7DAF985301",  // iMac14,3
        "Mac-C3EC7CD22292981F",  // MacBookPro10,1
        "Mac-C9CF552659EA9913",  // ???
        "Mac-F221BEC8",          // MacPro5,1 (and MacPro4,1)
        "Mac-F221DCC8",          // iMac10,1
        "Mac-F42C88C8",          // MacPro3,1
        "Mac-FC02E91DDD3FA6A4",  // iMac13,2
        "Mac-2BD1B31983FE1663"   // MacBookPro11,3
    };
    for (size_t i = 0; i < arrsize(compatibleBoards); i++) {
        if (!strcmp(compatibleBoards[i], boardId)) {
            DBGLOG("wred", "disabling nvidia patches on model %s", boardId);
            return false;
        }
    }

    return true;
}

void WRed::wrapFramebufferInit(IOFramebuffer *fb) {
    bool backCopy = callbackWEG->gotConsoleVinfo &&
                    callbackWEG->resetFramebuffer == FB_COPY;
    bool zeroFill = callbackWEG->gotConsoleVinfo &&
                    callbackWEG->resetFramebuffer == FB_ZEROFILL;
    auto &info = callbackWEG->consoleVinfo;

    // Copy back usually happens in a separate call to frameBufferInit
    // Furthermore, v_baseaddr may not be available on subsequent calls, so we
    // have to copy
    if (backCopy && info.v_baseaddr) {
        // Note, this buffer is left allocated and never freed, yet there
        // actually is no way to free it.
        callbackWEG->consoleBuffer =
            Buffer::create<uint8_t>(info.v_rowbytes * info.v_height);
        if (callbackWEG->consoleBuffer)
            lilu_os_memcpy(callbackWEG->consoleBuffer,
                           reinterpret_cast<uint8_t *>(info.v_baseaddr),
                           info.v_rowbytes * info.v_height);
        else
            SYSLOG("wred", "console buffer allocation failure");
        // Even if we may succeed next time, it will be unreasonably dangerous
        info.v_baseaddr = 0;
    }

    uint8_t verboseBoot = *callbackWEG->gIOFBVerboseBootPtr;
    // For back copy we need a console buffer and no verbose
    backCopy = backCopy && callbackWEG->consoleBuffer && !verboseBoot;

    // Now check if the resolution and parameters match
    if (backCopy || zeroFill) {
        IODisplayModeID mode;
        IOIndex depth;
        IOPixelInformation pixelInfo;

        if (fb->getCurrentDisplayMode(&mode, &depth) == kIOReturnSuccess &&
            fb->getPixelInformation(mode, depth, kIOFBSystemAperture,
                                    &pixelInfo) == kIOReturnSuccess) {
            DBGLOG("wred", "fb info 1: %d:%d %u:%u:%u", mode, depth,
                   pixelInfo.bytesPerRow, pixelInfo.bytesPerPlane,
                   pixelInfo.bitsPerPixel);
            DBGLOG("wred", "fb info 2: %u:%u %s %u:%u:%u",
                   pixelInfo.componentCount, pixelInfo.bitsPerComponent,
                   pixelInfo.pixelFormat, pixelInfo.flags,
                   pixelInfo.activeWidth, pixelInfo.activeHeight);

            if (info.v_rowbytes != pixelInfo.bytesPerRow ||
                info.v_width != pixelInfo.activeWidth ||
                info.v_height != pixelInfo.activeHeight ||
                info.v_depth != pixelInfo.bitsPerPixel) {
                backCopy = zeroFill = false;
                DBGLOG("wred", "this display has different mode");
            }
        } else {
            DBGLOG("wred", "failed to obtain display mode");
            backCopy = zeroFill = false;
        }
    }

    // For whatever reason not resetting Intel framebuffer (back copy mode)
    // twice works better.
    if (!backCopy) *callbackWEG->gIOFBVerboseBootPtr = 1;
    FunctionCast(wrapFramebufferInit, callbackWEG->orgFramebufferInit)(fb);
    if (!backCopy) *callbackWEG->gIOFBVerboseBootPtr = verboseBoot;

    // Finish the framebuffer initialisation by filling with black or copying
    // the image back.
    if (FramebufferViewer::getVramMap(fb)) {
        auto src = reinterpret_cast<uint8_t *>(callbackWEG->consoleBuffer);
        auto dst = reinterpret_cast<uint8_t *>(
            FramebufferViewer::getVramMap(fb)->getVirtualAddress());
        if (backCopy) {
            DBGLOG("wred", "attempting to copy...");
            // Here you can actually draw at your will, but looks like only on
            // Intel. On AMD you technically can draw too, but it happens for a
            // very short while, and is not worth it.
            lilu_os_memcpy(dst, src, info.v_rowbytes * info.v_height);
        } else if (zeroFill) {
            // On AMD we do a zero-fill to ensure no visual glitches.
            DBGLOG("wred", "doing zero-fill...");
            memset(dst, 0, info.v_rowbytes * info.v_height);
        }
    }
}

bool WRed::wrapGraphicsPolicyStart(IOService *that, IOService *provider) {
    auto boardIdentifier = BaseDeviceInfo::get().boardIdentifier;

    DBGLOG("wred", "agdp fix got board-id %s", boardIdentifier);
    auto oldConfigMap =
        OSDynamicCast(OSDictionary, that->getProperty("ConfigMap"));
    if (oldConfigMap) {
        auto rawConfigMap = oldConfigMap->copyCollection();
        if (rawConfigMap) {
            auto newConfigMap = OSDynamicCast(OSDictionary, rawConfigMap);
            if (newConfigMap) {
                auto none = OSString::withCString("none");
                if (none) {
                    newConfigMap->setObject(boardIdentifier, none);
                    none->release();
                    that->setProperty("ConfigMap", newConfigMap);
                }
            } else {
                SYSLOG("wred", "agdp fix failed to clone ConfigMap");
            }
            rawConfigMap->release();
        }
    } else {
        SYSLOG("wred", "agdp fix failed to obtain valid ConfigMap");
    }

    bool result =
        FunctionCast(wrapGraphicsPolicyStart,
                     callbackWEG->orgGraphicsPolicyStart)(that, provider);
    DBGLOG("wred", "agdp start returned %d", result);

    return result;
}

bool WRed::wrapApplePanelSetDisplay(IOService *that, IODisplay *display) {
    if (!callbackWEG->applePanelDisplaySet) {
        callbackWEG->applePanelDisplaySet = true;
        auto panels =
            OSDynamicCast(OSDictionary, that->getProperty("ApplePanels"));
        if (panels) {
            auto rawPanels = panels->copyCollection();
            panels = OSDynamicCast(OSDictionary, rawPanels);

            if (panels) {
                const char *customName = nullptr;
                if (callbackWEG->appleBacklightCustomName != nullptr) {
                    auto length =
                        callbackWEG->appleBacklightCustomName->getLength();
                    const char *customNameBytes = static_cast<const char *>(
                        callbackWEG->appleBacklightCustomName
                            ->getBytesNoCopy());
                    if (length > 0 && customNameBytes[length - 1] == '\0')
                        customName = customNameBytes;
                }

                for (auto &entry : appleBacklightData) {
                    if (customName != nullptr &&
                        strcmp(customName, entry.deviceName) == 0) {
                        panels->setObject(
                            entry.deviceName,
                            callbackWEG->appleBacklightCustomData);
                        DBGLOG("wred", "using custom panel data for %s device",
                               entry.deviceName);
                    } else {
                        auto pd = OSData::withBytes(entry.deviceData,
                                                    sizeof(entry.deviceData));
                        if (pd) {
                            panels->setObject(entry.deviceName, pd);
                            // No release required by current AppleBacklight
                            // implementation.
                        } else {
                            SYSLOG("wred",
                                   "panel start cannot allocate %s data",
                                   entry.deviceName);
                        }
                    }
                }
                that->setProperty("ApplePanels", panels);
            }

            if (rawPanels) {
                rawPanels->release();
            }
        } else {
            SYSLOG("wred", "panel start has no panels");
        }
    }

    bool result =
        FunctionCast(wrapApplePanelSetDisplay,
                     callbackWEG->orgApplePanelSetDisplay)(that, display);
    DBGLOG("wred", "panel display set returned %d", result);

    return result;
}

bool WRed::getVideoArgument(DeviceInfo *info, const char *name, void *bootarg,
                            int size) {
    if (PE_parse_boot_argn(name, bootarg, size)) return true;

    for (size_t i = 0; i < info->videoExternal.size(); i++) {
        auto prop = OSDynamicCast(
            OSData, info->videoExternal[i].video->getProperty(name));
        auto propSize = prop ? prop->getLength() : 0;
        if (propSize > 0 && propSize <= size) {
            lilu_os_memcpy(bootarg, prop->getBytesNoCopy(), propSize);
            memset(static_cast<uint8_t *>(bootarg) + propSize, 0,
                   size - propSize);
            return true;
        }
    }

    if (info->videoBuiltin) {
        auto prop =
            OSDynamicCast(OSData, info->videoBuiltin->getProperty(name));
        auto propSize = prop ? prop->getLength() : 0;
        if (propSize > 0 && propSize <= size) {
            lilu_os_memcpy(bootarg, prop->getBytesNoCopy(), propSize);
            memset(static_cast<uint8_t *>(bootarg) + propSize, 0,
                   size - propSize);
            return true;
        }
    }

    return false;
}
