import configure

class Configure(configure.Configure):
  def __init__(self, framework):
    configure.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def setOutput(self):
    #self.addDefine('HAVE_PVODE', 0)
    self.addSubstitution('PVODE_INCLUDE', '', 'The PVODE include flags')
    self.addSubstitution('PVODE_LIB', '', 'The PVODE library flags')
    return

  def configure(self):
    self.setOutput()
    return
