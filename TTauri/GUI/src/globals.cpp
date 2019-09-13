// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/GUI/globals.hpp"
#include "TTauri/GUI/Instance.hpp"
#include "TTauri/Required/globals.hpp"
#include "TTauri/Time/globals.hpp"
#include "TTauri/Diagnostic/globals.hpp"
#include "TTauri/Foundation/globals.hpp"
#include "TTauri/Config/globals.hpp"
#include "TTauri/Draw/globals.hpp"

#include "shaders/PipelineImage.vert.spv.inl"
#include "shaders/PipelineImage.frag.spv.inl"
#include "shaders/PipelineFlat.vert.spv.inl"
#include "shaders/PipelineFlat.frag.spv.inl"


namespace TTauri::GUI {

#if OPERATING_SYSTEM == OS_WINDOWS
GUIGlobals::GUIGlobals(InstanceDelegate *instance_delegate, HINSTANCE hInstance, int nCmdShow) :
    instance_delegate(instance_delegate), hInstance(hInstance), nCmdShow(nCmdShow)
#else
GUIGlobals::GUIGlobals(InstanceDelegate *instance_delegate) :
    instance_delegate(instance_delegate)
#endif
{
    required_assert(Required_globals != nullptr);
    required_assert(Time_globals != nullptr);
    required_assert(Diagnostic_globals != nullptr);
    required_assert(Foundation_globals != nullptr);
    required_assert(Config::Config_globals != nullptr);
    required_assert(Draw::Draw_globals != nullptr);
    required_assert(GUI_globals == nullptr);
    GUI_globals = this;

    Foundation_globals->addStaticResource(PipelineImage_vert_spv_filename, PipelineImage_vert_spv_bytes);
    Foundation_globals->addStaticResource(PipelineImage_frag_spv_filename, PipelineImage_frag_spv_bytes);
    Foundation_globals->addStaticResource(PipelineFlat_vert_spv_filename, PipelineFlat_vert_spv_bytes);
    Foundation_globals->addStaticResource(PipelineFlat_frag_spv_filename, PipelineFlat_frag_spv_bytes);
}

GUIGlobals::~GUIGlobals()
{
    delete _instance;

    required_assert(GUI_globals == this);
    GUI_globals = nullptr;
}

Instance &GUIGlobals::instance()
{
    if (!_instance) {
        let lock = std::scoped_lock(mutex);
        if (!_instance) {
            required_assert(instance_delegate != nullptr);
            _instance = new Instance(instance_delegate);
        }
    }
    return *_instance;
}

}