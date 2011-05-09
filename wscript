srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

nonmac = False

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check(lib='portaudio', uselib_store='PA')
  conf.check(lib='sndfile', uselib_store='SF')


def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.uselib = ['PA', 'SF']
  obj.target = 'nodepa'
  obj.source = 'nodepa.cc'
