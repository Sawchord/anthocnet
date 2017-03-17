# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('anthocnet', ['core', 'internet', 'wifi'])
    module.source = [
        'model/anthocnet.cc',
        'model/anthocnet-config.cc',
        'model/anthocnet-rtable.cc',
        'model/anthocnet-pcache.cc',
        'model/anthocnet-packet.cc',
        'helper/anthocnet-helper.cc',
        
        'model/sim-database.cc',
        'model/sim-app.cc',
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
        'helper/anthocnet-helper.h',
        
        'model/sim-database.h',
        'model/sim-app.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

