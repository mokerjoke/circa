
import os,sys, glob

def create_windows_env(releaseBuild):
    env = Environment(platform = 'nt')
    env.Append(CPPFLAGS=['/EHsc'])

    if not releaseBuild:
        env.Append(LINKFLAGS = ['/DEBUG'])  # enable debugging information
        env.Append(LINKFLAGS = ['/NODEFAULTLIB:"LIBCMT.LIB"'])
        env.Append(LINKFLAGS = ['/NODEFAULTLIB:"LIBCPMT.LIB"'])
        env.Append(LINKFLAGS = ['/MACHINE:X86'])
        env.Append(CCFLAGS = ['/MD'])        # MSVCRT.LIB & MSVCPRT.LIB
        env.Append(CCFLAGS = ['/Od'])        # Disable optimizations
        env.Append(CCFLAGS = ['/Z7'])

    return env
        
def create_mac_env(releaseBuild):
    env = Environment()
    # Python support
    #env.Append(CPPPATH = ["/System/Library/Frameworks/Python.framework/Versions/2.5/Headers"])

    if not releaseBuild:
        env.Append(CPPFLAGS=['-ggdb'])
        # env.Append(CPPFLAGS=['-bundle'])
        # env.Append(CPPFLAGS=['-flat_namespace'])

    return env

RELEASE_BUILD = False
SOURCE_DIRECTORY = "src"

env = create_mac_env(RELEASE_BUILD)
env.Append(CPPDEFINES = ["_DEBUG"])
env.Append(CPPDEFINES = ["DEBUG"])

env.BuildDir('build', 'src')
env.Append(CPPPATH = ['src'])

# Find source files
excludeFromLibrary = ['test_program.cpp']
buildFiles = []
for (dirpath, dirnames, filenames) in os.walk('src'):
    for file in filenames:
        if file in excludeFromLibrary:
            continue
        fullpath = os.path.join(dirpath, file)
        fullpath = fullpath.replace("src","build")

        if fullpath.endswith('.cpp'):
            buildFiles.append(fullpath)
    
libcirca = env.SharedLibrary('circa', buildFiles)

circaBinary = env.Program('circa', 'build/main.cpp', LIBS=[libcirca])

env.Install('bin', circaBinary)

env.SetOption('num_jobs', 2)


