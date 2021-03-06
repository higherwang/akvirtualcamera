# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2020  Gonzalo Exequiel Pedone
#
# akvirtualcamera is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# akvirtualcamera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

include(../cmio.pri)

CONFIG += \
    staticlib \
    create_prl \
    no_install_prl
CONFIG -= qt

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = VCamIPC

TEMPLATE = lib

LIBS = \
    -L$${OUT_PWD}/../../VCamUtils/$${BIN_DIR} -lVCamUtils \
    -framework Foundation

OBJECTIVE_SOURCES = \
    src/ipcbridge.mm

HEADERS =  \
    ../../ipcbridge.h

INCLUDEPATH += \
    .. \
    ../..
