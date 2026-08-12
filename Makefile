# =============================================================================
# fish_cam_rpi - Makefile
#
# Targets:
#   all       Build the binary (default)
#   run       Build and run the default capture
#   demo      Build and run in demo mode (no camera needed)
#   test      Build and run the unit tests
#   install   Install binary and public headers into $(PREFIX)
#   uninstall Remove installed artifacts
#   info      Print resolved compiler/linker configuration
#   clean     Remove obj/ and bin/
#   distclean Remove obj/, bin/, captures/ and logs/
# =============================================================================

TARGET      := bin/fish_cam_rpi
PROJECT     := fish_cam_rpi
VERSION     := $(shell cat VERSION 2>/dev/null || echo 0.1.0)

CXX         ?= g++
CXXSTD      := -std=c++17
OPT         ?= -O2
CXXFLAGS    := $(CXXSTD) $(OPT) -Wall -Wextra -MMD -MP
INCLUDES    := -Iinclude -Isrc

PKG_CONFIG  := pkg-config

# --- Dependency discovery (pkg-config with conservative fallbacks) ----------
OPENCV_FLAGS := $(shell $(PKG_CONFIG) --cflags opencv4 2>/dev/null || \
                        $(PKG_CONFIG) --cflags opencv 2>/dev/null)
OPENCV_LIBS  := $(shell $(PKG_CONFIG) --libs opencv4 2>/dev/null || \
                        $(PKG_CONFIG) --libs opencv 2>/dev/null)
RASPICAM_CFLAGS := $(shell $(PKG_CONFIG) --cflags raspicam 2>/dev/null)
RASPICAM_LIBS  := $(shell $(PKG_CONFIG) --libs raspicam raspicam_cv 2>/dev/null)

ifeq ($(strip $(OPENCV_FLAGS)),)
OPENCV_FLAGS := -I/usr/include/opencv4
OPENCV_LIBS  := -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio
endif

# If raspicam is not announced by pkg-config, look for it in /usr/local (a
# source build installed with `cmake --install` but outside pkg-config's path).
ifeq ($(strip $(RASPICAM_LIBS)),)
RASPICAM_LIBS := $(shell ls /usr/local/lib/libraspicam_cv.so 2>/dev/null)
endif

# When raspicam is unavailable (e.g. Raspberry Pi OS Bookworm, which removed
# the legacy MMAL stack), build the OpenCV V4L2 camera backend instead.
ifeq ($(strip $(RASPICAM_LIBS)),)
$(info [!] raspicam not found - building with the OpenCV V4L2 camera backend)
CXXFLAGS += -DFISH_CAM_USE_OPENCV_BACKEND
else
CXXFLAGS += $(RASPICAM_CFLAGS)
endif

CXXFLAGS += $(INCLUDES) $(OPENCV_FLAGS) -DFISH_CAM_VERSION=\"$(VERSION)\"
LDFLAGS  := -pthread
LIBS     := $(RASPICAM_LIBS) $(OPENCV_LIBS)

# --- Sources and objects (obj/ mirrors the src/ tree) -----------------------
SRCS := $(shell find src -name '*.cpp' | sort)
OBJS := $(patsubst src/%.cpp,obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
CORE_OBJS := $(filter-out obj/main.o,$(OBJS))

# --- Unit tests --------------------------------------------------------------
UNIT_TEST_SRCS := $(wildcard tests/unit_tests/*.cpp)
UNIT_TEST_BINS := $(patsubst tests/unit_tests/%.cpp,bin/%.test,$(UNIT_TEST_SRCS))

PREFIX      ?= /usr/local
BINDIR      := $(PREFIX)/bin
INCLUDEDIR  := $(PREFIX)/include/fish_cam

.DEFAULT_GOAL := all

.PHONY: all run demo test install uninstall info version clean distclean

# =============================================================================
# Build
# =============================================================================

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)
	@echo "Built $@"

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

# =============================================================================
# Run
# =============================================================================

run: all
	./$(TARGET)

demo: all
	./$(TARGET) --demo

# =============================================================================
# Tests
# =============================================================================

bin/%.test: tests/unit_tests/%.cpp $(CORE_OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< $(CORE_OBJS) -o $@ $(LDFLAGS) $(LIBS)
	./$@

test: $(UNIT_TEST_BINS)

# =============================================================================
# Install
# =============================================================================

install: $(TARGET)
	install -d $(BINDIR) $(INCLUDEDIR)/camera $(INCLUDEDIR)/config \
	           $(INCLUDEDIR)/image $(INCLUDEDIR)/timestamp $(INCLUDEDIR)/utils
	install -m 755 $(TARGET) $(BINDIR)/$(PROJECT)
	install -m 644 include/fish_cam.hpp $(INCLUDEDIR)/
	install -m 644 src/camera/CameraManager.hpp $(INCLUDEDIR)/camera/
	install -m 644 src/config/ConfigManager.hpp $(INCLUDEDIR)/config/
	install -m 644 src/image/ImageProcessor.hpp $(INCLUDEDIR)/image/
	install -m 644 src/timestamp/TimestampManager.hpp $(INCLUDEDIR)/timestamp/
	install -m 644 src/utils/Logger.hpp $(INCLUDEDIR)/utils/
	@echo "Installed $(BINDIR)/$(PROJECT) and headers to $(INCLUDEDIR)"

uninstall:
	rm -f $(BINDIR)/$(PROJECT)
	rm -rf $(INCLUDEDIR)

# =============================================================================
# Utility
# =============================================================================

info:
	@echo "CXX       = $(CXX)"
	@echo "VERSION   = $(VERSION)"
	@echo "CXXFLAGS  = $(CXXFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "LIBS      = $(LIBS)"
	@echo "SRCS      = $(SRCS)"
	@echo "OBJS      = $(OBJS)"
	@echo "TESTS     = $(UNIT_TEST_BINS)"

version:
	@echo "fish_cam_rpi $(VERSION)"

clean:
	rm -rf obj bin

distclean: clean
	rm -rf captures logs
