SHELL = cmd.exe

CXX      = cl.exe
CXXFLAGS = /Zi /EHsc /Iheaders /nologo  # /Zi for debug
LDFLAGS  = /nologo /DEBUG /PDB:compiler.pdb # debug pdb

SRCDIR = src
OBJDIR = obj

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.obj,$(SOURCES))

TARGET = compiler.exe

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) /Fe$@ $(OBJECTS) /link $(LDFLAGS)

$(OBJDIR)/%.obj: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) /c $< /Fo$@

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	-@cmd.exe /C "rd /S /Q $(OBJDIR)"
	-@cmd.exe /C "del /S /Q compiler.*"
	-@cmd.exe /C "del /S /Q *.pdb"

.PHONY: all clean