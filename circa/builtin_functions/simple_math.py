import circa_function

class Add(circa_function.BaseFunction):
  def init(self):
    self.name = "add"
    self.pureFunction = True
    self.inputType = float
    self.outputType = float

  def evaluate(self, term):
    term.value = float(term.inputs[0]) + float(term.inputs[1])

class Sub(circa_function.BaseFunction):
  def init(self):
    self.name = "sub"
    self.pureFunction = True
    self.inputType = float
    self.outputType = float

  def evaluate(self, term):
    term.value = float(term.inputs[0]) - float(term.inputs[1])

class Mult(circa_function.BaseFunction):
  def init(self):
    self.name = "mult"
    self.pureFunction = True
    self.inputType = float
    self.outputType = float

  def evaluate(self, term):
    term.value = float(term.inputs[0]) * float(term.inputs[1])

class Div(circa_function.BaseFunction):
  def init(self):
    self.name = "div"
    self.pureFunction = True
    self.inputType = float
    self.outputType = float

  def evaluate(self, term):
    term.value = float(term.inputs[0]) / float(term.inputs[1])
    
class Blend(circa_function.BaseFunction):
  def init(self):
    self.name = "blend"
    self.pureFunction = True
    self.inputType = float
    self.outputType = float

  def evaluate(self, term):
    blend_value = float(term.inputs[2])
    term.value = float(term.inputs[0]) * (1 - blend_value) + float(term.inputs[1]) * blend_value

ADD = Add()
SUB = Sub()
MULT = Mult()
DIV = Div()
BLEND = Blend()

