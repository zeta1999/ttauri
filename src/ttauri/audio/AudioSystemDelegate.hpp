// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "AudioBlock.hpp"

namespace tt {

class AudioSystemDelegate {
public:
    AudioSystemDelegate() noexcept = default;

    AudioSystemDelegate(AudioSystemDelegate const &) = delete;
    AudioSystemDelegate &operator=(AudioSystemDelegate const &) = delete;
    AudioSystemDelegate(AudioSystemDelegate &&) = delete;
    AudioSystemDelegate &operator=(AudioSystemDelegate &&) = delete;

    /*! Called when the device list has changed.
     * This can happen when external devices are connected or disconnected.
     */
    virtual void audioDeviceListChanged() = 0;
};

}