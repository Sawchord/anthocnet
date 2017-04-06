# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def configure(conf):
    #conf.env.append_value("CXXFLAGS", ["-I../src/anthocnet/fuzzylite/fuzzylite", "-L../src/anthocnet/fuzzylite/fuzzylite/debug/bin", "-lfuzzylite-static-debug"])
    #conf.env.prepend_value("LDFLAGS", ["-L../src/anthocnet/fuzzylite/fuzzylite/debug/bin", "-lfuzzylite-static-debug"])
    pass

def build(bld):
    module = bld.create_ns3_module('anthocnet', ['core', 'internet', 'wifi'])
    
    module.env.append_value("CXXFLAGS", ["-I../src/anthocnet/fuzzylite/fuzzylite"])
    module.env.append_value("LINKFLAGS", ["-L../src/anthocnet/fuzzylite/fuzzylite/debug/bin"])
    module.env.append_value("LIB", ["fuzzylite-static-debug"])
    
    module.source = [
        'model/anthocnet.cc',
        'model/anthocnet-config.cc',
        'model/anthocnet-rtable.cc',
        'model/anthocnet-pcache.cc',
        'model/anthocnet-packet.cc',
        'model/anthocnet-fis.cc',
        'model/anthocnet-stat.cc',
        'helper/anthocnet-helper.cc',
        
        'model/sim-database.cc',
        'model/sim-app.cc',
        'helper/sim-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('anthocnet')
    module_test.source = [
        'test/anthocnet-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'anthocnet'
    headers.source = [
        'model/anthocnet.h',
        'model/anthocnet-config.h',
        'model/anthocnet-rtable.h',
        'model/anthocnet-pcache.h',
        'model/anthocnet-packet.h',
        'model/anthocnet-fis.h',
        'model/anthocnet-stat.h',
        'helper/anthocnet-helper.h',
        
        'model/sim-database.h',
        'model/sim-app.h',
        'helper/sim-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

