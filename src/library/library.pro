QT += opengl

TEMPLATE = lib
DEFINES += LIBRARY_LIBRARY
DEFINES += CORE_UTILS=\\\"C:\\\\\\\\mingw\\\\\\\\coreutils-5.3.0\\\\\\\\bin\\\"

# CONFIG += c++11

QMAKE_CXXFLAGS += -Wno-return-type -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-but-set-parameter

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += C:/Qt/Tools/mingw810_64/x86_64-w64-mingw32/include/GL

SOURCES += \
    agent/agent.cpp \
    agent/AgentAttachedData.cpp \
    agent/BeingCarriedSensor.cpp \
    agent/CarryingSensor.cpp \
    agent/EnergySensor.cpp \
    agent/LifeSpan.cpp \
    agent/MateWaitSensor.cpp \
    agent/Metabolism.cpp \
    agent/RandomSensor.cpp \
    agent/Retina.cpp \
    agent/RqSensor.cpp \
    agent/SpeedSensor.cpp \
    brain/Brain.cpp \
    brain/FiringRateModel.cpp \
    brain/Nerve.cpp \
    brain/NervousSystem.cpp \
    brain/RqNervousSystem.cpp \
    brain/SpikingModel.cpp \
    brain/groups/GroupsBrain.cpp \
    brain/sheets/SheetsBrain.cpp \
    brain/sheets/SheetsModel.cpp \
    complexity/adami.cpp \
    complexity/complexity_algorithm.cpp \
    complexity/complexity_brain.cpp \
    complexity/complexity_motion.cpp \
    environment/barrier.cpp \
    environment/brick.cpp \
    environment/BrickPatch.cpp \
    environment/Energy.cpp \
    environment/food.cpp \
    environment/FoodPatch.cpp \
    environment/FoodType.cpp \
    environment/Patch.cpp \
    genome/Gene.cpp \
    genome/GeneSchema.cpp \
    genome/Genome.cpp \
    genome/GenomeLayout.cpp \
    genome/GenomeSchema.cpp \
    genome/GenomeUtil.cpp \
    genome/SeparationCache.cpp \
    genome/groups/GroupsGene.cpp \
    genome/groups/GroupsGenome.cpp \
    genome/groups/GroupsGenomeSchema.cpp \
    genome/groups/GroupsSynapseType.cpp \
    genome/sheets/SheetsCrossover.cpp \
    genome/sheets/SheetsGenome.cpp \
    genome/sheets/SheetsGenomeSchema.cpp \
    graphics/gcamera.cpp \
    graphics/glight.cpp \
    graphics/gline.cpp \
    graphics/gmisc.cpp \
    graphics/gobject.cpp \
    graphics/gpoint.cpp \
    graphics/gpolygon.cpp \
    graphics/graphics.cpp \
    graphics/grect.cpp \
    graphics/gscene.cpp \
    graphics/gsquare.cpp \
    graphics/gstage.cpp \
    logs/Logger.cpp \
    logs/Logs.cpp \
    monitor/AgentTracker.cpp \
    monitor/CameraController.cpp \
    monitor/Monitor.cpp \
    monitor/MonitorManager.cpp \
    monitor/MovieController.cpp \
    monitor/SceneRenderer.cpp \
    proplib/builder.cpp \
    proplib/convert.cpp \
    proplib/cppprops.cpp \
    proplib/dom.cpp \
    proplib/editor.cpp \
    proplib/expression.cpp \
    proplib/interpreter.cpp \
    proplib/overlay.cpp \
    proplib/parser.cpp \
    proplib/schema.cpp \
    proplib/state.cpp \
    proplib/writer.cpp \
    sim/debug.cpp \
    sim/EatStatistics.cpp \
    sim/FittestList.cpp \
    sim/GeneStats.cpp \
    sim/globals.cpp \
    sim/Scheduler.cpp \
    sim/simtypes.cpp \
    sim/Simulation.cpp \
    utils/AbstractFile.cpp \
    utils/analysis.cpp \
    utils/datalib.cpp \
    utils/distributions.cpp \
    utils/drand48.cpp \
    utils/error.cpp \
    utils/indexlist.cpp \
    utils/misc.cpp \
    utils/objectxsortedlist.cpp \
    utils/PwMovieUtils.cpp \
    utils/RandomNumberGenerator.cpp \
    utils/resource.cpp \
    utils/Resources.cpp \
    utils/Scalar.cpp \
    utils/ThreadPool.cpp \
    utils/Variant.cpp \
    windows/dlfcn.c \
    windows/link.c \
    renderer/qt/PwMovieQGLPixelBufferRecorder.cpp \
    renderer/qt/QtAgentPovRenderer.cpp \
    renderer/qt/QtSceneRenderer.cpp

HEADERS += \
    library_global.h \
    agent/agent.h \
    agent/AgentAttachedData.h \
    agent/AgentListener.h \
    agent/AgentPovRenderer.h \
    agent/BeingCarriedSensor.h \
    agent/CarryingSensor.h \
    agent/EnergySensor.h \
    agent/LifeSpan.h \
    agent/MateWaitSensor.h \
    agent/Metabolism.h \
    agent/RandomSensor.h \
    agent/Retina.h \
    agent/RqSensor.h \
    agent/SpeedSensor.h \
    brain/BaseNeuronModel.h \
    brain/Brain.h \
    brain/FiringRateModel.h \
    brain/Nerve.h \
    brain/NervousSystem.h \
    brain/NeuralNetRenderer.h \
    brain/NeuronModel.h \
    brain/RqNervousSystem.h \
    brain/Sensor.h \
    brain/SpikingModel.h \
    brain/groups/GroupsBrain.h \
    brain/groups/GroupsNeuralNetRenderer.h \
    brain/sheets/SheetsBrain.h \
    brain/sheets/SheetsModel.h \
    complexity/adami.h \
    complexity/complexity.h \
    complexity/complexity_algorithm.h \
    complexity/complexity_brain.h \
    complexity/complexity_motion.h \
    environment/barrier.h \
    environment/brick.h \
    environment/BrickPatch.h \
    environment/Energy.h \
    environment/food.h \
    environment/FoodPatch.h \
    environment/FoodType.h \
    environment/Patch.h \
    genome/Gene.h \
    genome/GeneSchema.h \
    genome/Genome.h \
    genome/GenomeLayout.h \
    genome/GenomeSchema.h \
    genome/GenomeUtil.h \
    genome/NeurGroupType.h \
    genome/NeuronType.h \
    genome/SeparationCache.h \
    genome/groups/GroupsGene.h \
    genome/groups/GroupsGenome.h \
    genome/groups/GroupsGenomeSchema.h \
    genome/groups/GroupsSynapseType.h \
    genome/sheets/SheetsCrossover.h \
    genome/sheets/SheetsGenome.h \
    genome/sheets/SheetsGenomeSchema.h \
    graphics/gcamera.h \
    graphics/glight.h \
    graphics/gline.h \
    graphics/gmisc.h \
    graphics/gobject.h \
    graphics/gpoint.h \
    graphics/gpolygon.h \
    graphics/graphics.h \
    graphics/grect.h \
    graphics/gscene.h \
    graphics/gsquare.h \
    graphics/gstage.h \
    logs/Logger.h \
    logs/Logs.h \
    monitor/AgentTracker.h \
    monitor/CameraController.h \
    monitor/Monitor.h \
    monitor/MonitorManager.h \
    monitor/MovieController.h \
    monitor/MovieRecorder.h \
    monitor/SceneRenderer.h \
    proplib/builder.h \
    proplib/convert.h \
    proplib/cppprops.h \
    proplib/dom.h \
    proplib/editor.h \
    proplib/expression.h \
    proplib/interpreter.h \
    proplib/overlay.h \
    proplib/parser.h \
    proplib/proplib.h \
    proplib/schema.h \
    proplib/state.h \
    proplib/writer.h \
    sim/debug.h \
    sim/Domain.h \
    sim/EatStatistics.h \
    sim/FittestList.h \
    sim/GeneStats.h \
    sim/globals.h \
    sim/Scheduler.h \
    sim/simconst.h \
    sim/simtypes.h \
    sim/Simulation.h \
    utils/AbstractFile.h \
    utils/analysis.h \
    utils/datalib.h \
    utils/distributions.h \
    utils/drand48.h \
    utils/error.h \
    utils/Events.h \
    utils/gdlink.h \
    utils/graybin.h \
    utils/indexlist.h \
    utils/misc.h \
    utils/next_combination.h \
    utils/objectlist.h \
    utils/objectxsortedlist.h \
    utils/PwMovieUtils.h \
    utils/RandomNumberGenerator.h \
    utils/resource.h \
    utils/Resources.h \
    utils/Scalar.h \
    utils/Signal.h \
    utils/ThreadPool.h \
    utils/Variant.h \
    windows/dlfcn.h \
    windows/link.h \
    renderer/qt/PwMovieQGLPixelBufferRecorder.h \
    renderer/qt/QtAgentPovRenderer.h \
    renderer/qt/QtSceneRenderer.h

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -lopengl32

win32: LIBS += -lglu32

win32: LIBS += -lgsl

win32: LIBS += -lz

win32: LIBS += -lws2_32
