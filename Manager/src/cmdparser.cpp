/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <algorithm>
#include <codecvt>
#include <csignal>
#include <iostream>
#include <functional>
#include <locale>
#include <sstream>

#include "cmdparser.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/settings.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/logger.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this->d, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam {
    using StringMatrix = std::vector<StringVector>;
    using VideoFormatMatrix = std::vector<std::vector<VideoFormat>>;

    struct CmdParserFlags
    {
        StringVector flags;
        std::string value;
        std::string helpString;
    };

    struct CmdParserCommand
    {
        std::string command;
        std::string arguments;
        std::string helpString;
        ProgramOptionsFunc func;
        std::vector<CmdParserFlags> flags;
    };

    class CmdParserPrivate
    {
        public:
            std::vector<CmdParserCommand> m_commands;
            IpcBridge m_ipcBridge;
            bool m_parseable {false};

            static const std::map<ControlType, std::string> &typeStrMap();
            std::string basename(const std::string &path);
            void printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                            size_t indent);
            size_t maxCommandLength();
            size_t maxArgumentsLength();
            size_t maxFlagsLength(const std::vector<CmdParserFlags> &flags);
            size_t maxFlagsValueLength(const std::vector<CmdParserFlags> &flags);
            size_t maxColumnLength(const StringVector &table,
                                   size_t width,
                                   size_t column);
            std::vector<size_t> maxColumnsLength(const StringVector &table,
                                                 size_t width);
            void drawTableHLine(const std::vector<size_t> &columnsLength);
            void drawTable(const StringVector &table, size_t width);
            CmdParserCommand *parserCommand(const std::string &command);
            const CmdParserFlags *parserFlag(const std::vector<CmdParserFlags> &cmdFlags,
                                             const std::string &flag);
            bool containsFlag(const StringMap &flags,
                              const std::string &command,
                              const std::string &flagAlias);
            std::string flagValue(const StringMap &flags,
                                  const std::string &command,
                                  const std::string &flagAlias);
            int defaultHandler(const StringMap &flags,
                               const StringVector &args);
            int showHelp(const StringMap &flags, const StringVector &args);
            int showDevices(const StringMap &flags, const StringVector &args);
            int addDevice(const StringMap &flags, const StringVector &args);
            int removeDevice(const StringMap &flags, const StringVector &args);
            int removeDevices(const StringMap &flags, const StringVector &args);
            int showDeviceDescription(const StringMap &flags,
                                      const StringVector &args);
            int setDeviceDescription(const StringMap &flags,
                                     const StringVector &args);
            int showSupportedFormats(const StringMap &flags,
                                     const StringVector &args);
            int showFormats(const StringMap &flags, const StringVector &args);
            int addFormat(const StringMap &flags, const StringVector &args);
            int removeFormat(const StringMap &flags, const StringVector &args);
            int removeFormats(const StringMap &flags, const StringVector &args);
            int update(const StringMap &flags, const StringVector &args);
            int loadSettings(const StringMap &flags, const StringVector &args);
            int stream(const StringMap &flags, const StringVector &args);
            int showControls(const StringMap &flags, const StringVector &args);
            int readControl(const StringMap &flags, const StringVector &args);
            int writeControls(const StringMap &flags, const StringVector &args);
            int picture(const StringMap &flags, const StringVector &args);
            int setPicture(const StringMap &flags, const StringVector &args);
            int logLevel(const StringMap &flags, const StringVector &args);
            int setLogLevel(const StringMap &flags, const StringVector &args);
            int showClients(const StringMap &flags, const StringVector &args);
            void loadGenerals(Settings &settings);
            VideoFormatMatrix readFormats(Settings &settings);
            std::vector<VideoFormat> readFormat(Settings &settings);
            StringMatrix matrixCombine(const StringMatrix &matrix);
            void matrixCombineP(const StringMatrix &matrix,
                                size_t index,
                                StringVector combined,
                                StringMatrix &combinations);
            void createDevices(Settings &settings,
                               const VideoFormatMatrix &availableFormats);
            void createDevice(Settings &settings,
                              const VideoFormatMatrix &availableFormats);
            std::vector<VideoFormat> readDeviceFormats(Settings &settings,
                                                       const VideoFormatMatrix &availableFormats);
    };

    std::string operator *(const std::string &str, size_t n);
}

AkVCam::CmdParser::CmdParser()
{
    this->d = new CmdParserPrivate();
    this->d->m_commands.push_back({});
    this->setDefaultFuntion(AKVCAM_BIND_FUNC(CmdParserPrivate::defaultHandler));
    this->addFlags("",
                   {"-h", "--help"},
                   "Show help.");
    this->addFlags("",
                   {"-p", "--parseable"},
                   "Show parseable output.");
    this->addCommand("devices",
                     "",
                     "List devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDevices));
    this->addCommand("add-device",
                     "DESCRIPTION",
                     "Add a new device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::addDevice));
    this->addCommand("remove-device",
                     "DEVICE",
                     "Remove a device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeDevice));
    this->addCommand("remove-devices",
                     "",
                     "Remove all devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeDevices));
    this->addCommand("description",
                     "DEVICE",
                     "Show device description.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDeviceDescription));
    this->addCommand("set-description",
                     "DEVICE DESCRIPTION",
                     "Set device description.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setDeviceDescription));
    this->addCommand("supported-formats",
                     "",
                     "Show supported formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showSupportedFormats));
    this->addFlags("supported-formats",
                   {"-i", "--input"},
                   "Show supported input formats.");
    this->addFlags("supported-formats",
                   {"-o", "--output"},
                   "Show supported output formats.");
    this->addCommand("formats",
                     "DEVICE",
                     "Show device formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showFormats));
    this->addCommand("add-format",
                     "DEVICE FORMAT WIDTH HEIGHT FPS",
                     "Add a new device format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::addFormat));
    this->addFlags("add-format",
                   {"-i", "--index"},
                   "INDEX",
                   "Add format at INDEX.");
    this->addCommand("remove-format",
                     "DEVICE INDEX",
                     "Remove device format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeFormat));
    this->addCommand("remove-formats",
                     "DEVICE",
                     "Remove all device formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeFormats));
    this->addCommand("update",
                     "",
                     "Update devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::update));
    this->addCommand("load",
                     "SETTINGS.INI",
                     "Create devices from a setting file.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::loadSettings));
    this->addCommand("stream",
                     "DEVICE FORMAT WIDTH HEIGHT",
                     "Read frames from stdin and send them to the device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::stream));
    this->addCommand("controls",
                     "DEVICE",
                     "Show device controls.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showControls));
    this->addCommand("get-control",
                     "DEVICE CONTROL",
                     "Read device control.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::readControl));
    this->addFlags("get-control",
                   {"-c", "--description"},
                   "Show control description.");
    this->addFlags("get-control",
                   {"-t", "--type"},
                   "Show control type.");
    this->addFlags("get-control",
                   {"-m", "--min"},
                   "Show minimum value for the control.");
    this->addFlags("get-control",
                   {"-M", "--max"},
                   "Show maximum value for the control.");
    this->addFlags("get-control",
                   {"-s", "--step"},
                   "Show increment/decrement step for the control.");
    this->addFlags("get-control",
                   {"-d", "--default"},
                   "Show default value for the control.");
    this->addFlags("get-control",
                   {"-l", "--menu"},
                   "Show options of a memu type control.");
    this->addCommand("set-controls",
                     "DEVICE CONTROL_1=VALUE CONTROL_2=VALUE...",
                     "Write device controls values.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::writeControls));
    this->addCommand("picture",
                     "",
                     "Placeholder picture to show when no streaming.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::picture));
    this->addCommand("set-picture",
                     "FILE",
                     "Set placeholder picture.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setPicture));
    this->addCommand("loglevel",
                     "",
                     "Show current debugging level.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::logLevel));
    this->addCommand("set-loglevel",
                     "LEVEL",
                     "Set debugging level.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setLogLevel));
    this->addCommand("clients",
                     "",
                     "Show clients using the camera.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showClients));
}

AkVCam::CmdParser::~CmdParser()
{
    delete this->d;
}

int AkVCam::CmdParser::parse(int argc, char **argv)
{
    auto program = this->d->basename(argv[0]);
    auto command = &this->d->m_commands[0];
    StringMap flags;
    StringVector arguments {program};

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            auto flag = this->d->parserFlag(command->flags, arg);

            if (!flag) {
                if (command->command.empty())
                    std::cout << "Invalid option '"
                              << arg
                              << "'"
                              << std::endl;
                else
                    std::cout << "Invalid option '"
                              << arg << "' for '"
                              << command->command
                              << "'"
                              << std::endl;

                return -1;
            }

            std::string value;

            if (!flag->value.empty()) {
                auto next = i + 1;

                if (next < argc) {
                    value = argv[next];
                    i++;
                }
            }

            flags[arg] = value;
        } else {
            if (command->command.empty()) {
                if (!flags.empty()) {
                    auto result = command->func(flags, {program});

                    if (result < 0)
                        return result;

                    flags.clear();
                }

                auto cmd = this->d->parserCommand(arg);

                if (cmd) {
                    command = cmd;
                    flags.clear();
                } else {
                    std::cout << "Unknown command '" << arg << "'" << std::endl;

                    return -1;
                }
            } else {
                arguments.push_back(arg);
            }
        }
    }

    return command->func(flags, arguments);
}

void AkVCam::CmdParser::setDefaultFuntion(const ProgramOptionsFunc &func)
{
    this->d->m_commands[0].func = func;
}

void AkVCam::CmdParser::addCommand(const std::string &command,
                                   const std::string &arguments,
                                   const std::string &helpString,
                                   const ProgramOptionsFunc &func)
{
    auto it =
            std::find_if(this->d->m_commands.begin(),
                         this->d->m_commands.end(),
                         [&command] (const CmdParserCommand &cmd) -> bool {
        return cmd.command == command;
    });

    if (it == this->d->m_commands.end()) {
        this->d->m_commands.push_back({command,
                                       arguments,
                                       helpString,
                                       func,
                                       {}});
    } else {
        it->command = command;
        it->arguments = arguments;
        it->helpString = helpString;
        it->func = func;
    }
}

void AkVCam::CmdParser::addFlags(const std::string &command,
                                 const StringVector &flags,
                                 const std::string &value,
                                 const std::string &helpString)
{
    auto it =
            std::find_if(this->d->m_commands.begin(),
                         this->d->m_commands.end(),
                         [&command] (const CmdParserCommand &cmd) -> bool {
        return cmd.command == command;
    });

    if (it == this->d->m_commands.end())
        return;

    it->flags.push_back({flags, value, helpString});
}

void AkVCam::CmdParser::addFlags(const std::string &command,
                                 const StringVector &flags,
                                 const std::string &helpString)
{
    this->addFlags(command, flags, "", helpString);
}

const std::map<AkVCam::ControlType, std::string> &AkVCam::CmdParserPrivate::typeStrMap()
{
    static const std::map<ControlType, std::string> typeStr {
        {ControlTypeInteger, "Integer"},
        {ControlTypeBoolean, "Boolean"},
        {ControlTypeMenu   , "Menu"   },
    };

    return typeStr;
}

std::string AkVCam::CmdParserPrivate::basename(const std::string &path)
{
    auto rit =
            std::find_if(path.rbegin(),
                         path.rend(),
                         [] (char c) -> bool {
        return c == '/' || c == '\\';
    });

    auto program =
            rit == path.rend()?
                path:
                path.substr(path.size() + size_t(path.rbegin() - rit));

    auto it =
            std::find_if(program.begin(),
                         program.end(),
                         [] (char c) -> bool {
        return c == '.';
    });

    program =
            it == path.end()?
                program:
                program.substr(0, size_t(it - program.begin()));

    return program;
}

void AkVCam::CmdParserPrivate::printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                                          size_t indent)
{
    std::vector<char> spaces(indent, ' ');
    auto maxFlagsLen = this->maxFlagsLength(cmdFlags);
    auto maxFlagsValueLen = this->maxFlagsValueLength(cmdFlags);

    for (auto &flag: cmdFlags) {
        auto allFlags = join(flag.flags, ", ");
        std::cout << std::string(spaces.data(), indent)
                  << fill(allFlags, maxFlagsLen);

        if (maxFlagsValueLen > 0)
            std::cout << " " << fill(flag.value, maxFlagsValueLen);

        std::cout << "    "
                  << flag.helpString
                  << std::endl;
    }
}

size_t AkVCam::CmdParserPrivate::maxCommandLength()
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        length = std::max(cmd.command.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxArgumentsLength()
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        length = std::max(cmd.arguments.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(join(flag.flags, ", ").size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsValueLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(flag.value.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxColumnLength(const AkVCam::StringVector &table,
                                                 size_t width,
                                                 size_t column)
{
    size_t length = 0;
    size_t height = table.size() / width;

    for (size_t y = 0; y < height; y++) {
        auto &str = table[y * width + column];
        length = std::max(str.size(), length);
    }

    return length;
}

std::vector<size_t> AkVCam::CmdParserPrivate::maxColumnsLength(const AkVCam::StringVector &table,
                                                               size_t width)
{
    std::vector<size_t> lengths;

    for (size_t x = 0; x < width; x++)
        lengths.push_back(this->maxColumnLength(table, width, x));

    return lengths;
}

void AkVCam::CmdParserPrivate::drawTableHLine(const std::vector<size_t> &columnsLength)
{
    std::cout << '+';

    for (auto &len: columnsLength)
        std::cout << std::string("-") * (len + 2) << '+';

    std::cout << std::endl;
}

void AkVCam::CmdParserPrivate::drawTable(const AkVCam::StringVector &table,
                                         size_t width)
{
    size_t height = table.size() / width;
    auto columnsLength = this->maxColumnsLength(table, width);
    this->drawTableHLine(columnsLength);

    for (size_t y = 0; y < height; y++) {
        std::cout << "|";

        for (size_t x = 0; x < width; x++) {
            auto &element = table[x + y * width];
            std::cout << " " << fill(element, columnsLength[x]) << " |";
        }

        std::cout << std::endl;

        if (y == 0 && height > 1)
            this->drawTableHLine(columnsLength);
    }

    this->drawTableHLine(columnsLength);
}

AkVCam::CmdParserCommand *AkVCam::CmdParserPrivate::parserCommand(const std::string &command)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command)
            return &cmd;

    return nullptr;
}

const AkVCam::CmdParserFlags *AkVCam::CmdParserPrivate::parserFlag(const std::vector<CmdParserFlags> &cmdFlags,
                                                                   const std::string &flag)
{
    for (auto &flags: cmdFlags)
        for (auto &f: flags.flags)
            if (f == flag)
                return &flags;

    return nullptr;
}

bool AkVCam::CmdParserPrivate::containsFlag(const StringMap &flags,
                                            const std::string &command,
                                            const std::string &flagAlias)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command) {
            for (auto &flag: cmd.flags) {
                auto it = std::find(flag.flags.begin(),
                                    flag.flags.end(),
                                    flagAlias);

                if (it != flag.flags.end()) {
                    for (auto &f1: flags)
                        for (auto &f2: flag.flags)
                            if (f1.first == f2)
                                return true;

                    return false;
                }
            }

            return false;
        }

    return false;
}

std::string AkVCam::CmdParserPrivate::flagValue(const AkVCam::StringMap &flags,
                                                const std::string &command,
                                                const std::string &flagAlias)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command) {
            for (auto &flag: cmd.flags) {
                auto it = std::find(flag.flags.begin(),
                                    flag.flags.end(),
                                    flagAlias);

                if (it != flag.flags.end()) {
                    for (auto &f1: flags)
                        for (auto &f2: flag.flags)
                            if (f1.first == f2)
                                return f1.second;

                    return {};
                }
            }

            return {};
        }

    return {};
}

int AkVCam::CmdParserPrivate::defaultHandler(const StringMap &flags,
                                             const StringVector &args)
{
    if (flags.empty() || this->containsFlag(flags, "", "-h"))
        return this->showHelp(flags, args);

    if (this->containsFlag(flags, "", "-p"))
        this->m_parseable = true;

    return 0;
}

int AkVCam::CmdParserPrivate::showHelp(const StringMap &flags,
                                       const StringVector &args)
{
    UNUSED(flags);

    std::cout << args[0]
              << " [OPTIONS...] COMMAND [COMMAND_OPTIONS...] ..."
              << std::endl;
    std::cout << std::endl;
    std::cout << "AkVirtualCamera virtual device manager." << std::endl;
    std::cout << std::endl;
    std::cout << "General Options:" << std::endl;
    std::cout << std::endl;
    this->printFlags(this->m_commands[0].flags, 4);
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << std::endl;

    auto maxCmdLen = this->maxCommandLength();
    auto maxArgsLen = this->maxArgumentsLength();

    for (auto &cmd: this->m_commands) {
        if (cmd.command.empty())
            continue;

        std::cout << "    "
                  << fill(cmd.command, maxCmdLen)
                  << " "
                  << fill(cmd.arguments, maxArgsLen)
                  << "    "
                  << cmd.helpString << std::endl;

        if (!cmd.flags.empty())
            std::cout << std::endl;

        this->printFlags(cmd.flags, 8);

        if (!cmd.flags.empty())
            std::cout << std::endl;
    }

    return 0;
}

int AkVCam::CmdParserPrivate::showDevices(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    auto devices = this->m_ipcBridge.devices();

    if (devices.empty())
        return 0;

    if (this->m_parseable) {
        for (auto &device: devices)
            std::cout << device << std::endl;
    } else {
        std::vector<std::string> table {
            "Device",
            "Description"
        };
        auto columns = table.size();

        for (auto &device: devices) {
            table.push_back(device);
            table.push_back(this->m_ipcBridge.description(device));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::addDevice(const StringMap &flags,
                                        const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device description not provided." << std::endl;

        return -1;
    }

    auto deviceId = this->m_ipcBridge.addDevice(args[1]);

    if (deviceId.empty()) {
        std::cerr << "Failed to create device." << std::endl;

        return -1;
    }

    if (this->m_parseable)
        std::cout << deviceId << std::endl;
    else
        std::cout << "Device created as " << deviceId << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::removeDevice(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    this->m_ipcBridge.removeDevice(args[1]);

    return 0;
}

int AkVCam::CmdParserPrivate::removeDevices(const AkVCam::StringMap &flags,
                                            const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    auto devices = this->m_ipcBridge.devices();

    for (auto &device: devices)
        this->m_ipcBridge.removeDevice(device);

    return 0;
}

int AkVCam::CmdParserPrivate::showDeviceDescription(const StringMap &flags,
                                                    const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    std::cout << this->m_ipcBridge.description(args[1]) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setDeviceDescription(const AkVCam::StringMap &flags,
                                                   const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    this->m_ipcBridge.setDescription(deviceId, args[2]);

    return 0;
}

int AkVCam::CmdParserPrivate::showSupportedFormats(const StringMap &flags,
                                                   const StringVector &args)
{
    UNUSED(args);

    auto type =
            this->containsFlag(flags, "supported-formats", "-i")?
                IpcBridge::StreamTypeInput:
                IpcBridge::StreamTypeOutput;
    auto formats = this->m_ipcBridge.supportedPixelFormats(type);

    if (!this->m_parseable) {
        if (type == IpcBridge::StreamTypeInput)
            std::cout << "Input formats:" << std::endl;
        else
            std::cout << "Output formats:" << std::endl;

        std::cout << std::endl;
    }

    for (auto &format: formats)
        std::cout << VideoFormat::stringFromFourcc(format) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showFormats(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    if (this->m_parseable) {
        for  (auto &format: this->m_ipcBridge.formats(args[1]))
            std::cout << VideoFormat::stringFromFourcc(format.fourcc())
                      << ' '
                      << format.width()
                      << ' '
                      << format.height()
                      << ' '
                      << format.minimumFrameRate().num()
                      << ' '
                      << format.minimumFrameRate().den()
                      << std::endl;
    } else {
        int i = 0;

        for  (auto &format: this->m_ipcBridge.formats(args[1])) {
            std::cout << i
                      << ": "
                      << VideoFormat::stringFromFourcc(format.fourcc())
                      << ' '
                      << format.width()
                      << 'x'
                      << format.height()
                      << ' '
                      << format.minimumFrameRate().num()
                      << '/'
                      << format.minimumFrameRate().den()
                      << " FPS"
                      << std::endl;
            i++;
        }
    }

    return 0;
}

int AkVCam::CmdParserPrivate::addFormat(const StringMap &flags,
                                        const StringVector &args)
{
    if (args.size() < 6) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    auto format = VideoFormat::fourccFromString(args[2]);

    if (!format) {
        std::cerr << "Invalid pixel format." << std::endl;

        return -1;
    }

    auto formats = this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamTypeOutput);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        std::cerr << "Format not supported." << std::endl;

        return -1;
    }

    char *p = nullptr;
    auto width = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -1;
    }

    p = nullptr;
    auto height = strtoul(args[4].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -1;
    }

    Fraction fps(args[5]);

    if (fps.num() < 1 || fps.den() < 1) {
        std::cerr << "Invalid frame rate." << std::endl;

        return -1;
    }

    auto indexStr = this->flagValue(flags, "add-format", "-i");
    int index = -1;

    if (!indexStr.empty()) {
        p = nullptr;
        index = int(strtoul(indexStr.c_str(), &p, 10));

        if (*p) {
            std::cerr << "Index must be an unsigned integer." << std::endl;

            return -1;
        }
    }

    VideoFormat fmt(format, int(width), int(height), {fps});
    this->m_ipcBridge.addFormat(deviceId, fmt, index);

    return 0;
}

int AkVCam::CmdParserPrivate::removeFormat(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    char *p = nullptr;
    auto index = strtoul(args[2].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Index must be an unsigned integer." << std::endl;

        return -1;
    }

    auto formats = this->m_ipcBridge.formats(deviceId);

    if (index >= formats.size()) {
        std::cerr << "Index is out of range." << std::endl;

        return -1;
    }

    this->m_ipcBridge.removeFormat(deviceId, int(index));

    return 0;
}

int AkVCam::CmdParserPrivate::removeFormats(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    this->m_ipcBridge.setFormats(deviceId, {});

    return 0;
}

int AkVCam::CmdParserPrivate::update(const StringMap &flags,
                                     const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    this->m_ipcBridge.updateDevices();

    return 0;
}

int AkVCam::CmdParserPrivate::loadSettings(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Settings file not provided." << std::endl;

        return -1;
    }

    Settings settings;

    if (!settings.load(args[1])) {
        std::cerr << "Settings file not valid." << std::endl;

        return -1;
    }

    this->loadGenerals(settings);
    auto devices = this->m_ipcBridge.devices();

    for (auto &device: devices)
        this->m_ipcBridge.removeDevice(device);

    this->createDevices(settings, this->readFormats(settings));

    return 0;
}

int AkVCam::CmdParserPrivate::stream(const AkVCam::StringMap &flags,
                                     const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 5) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    auto format = VideoFormat::fourccFromString(args[2]);

    if (!format) {
        std::cerr << "Invalid pixel format." << std::endl;

        return -1;
    }

    auto formats = this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamTypeOutput);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        std::cerr << "Format not supported." << std::endl;

        return -1;
    }

    char *p = nullptr;
    auto width = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -1;
    }

    p = nullptr;
    auto height = strtoul(args[4].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -1;
    }

    VideoFormat fmt(format, int(width), int(height), {{30, 1}});

    if (!this->m_ipcBridge.deviceStart(deviceId, fmt)) {
        std::cerr << "Can't start stream." << std::endl;

        return -1;
    }

    static bool exit = false;
    auto signalHandler = [] (int) {
        exit = true;
    };
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    AkVCam::VideoFrame frame(fmt);
    size_t bufferSize = 0;

    do {
        std::cin.read(reinterpret_cast<char *>(frame.data().data()
                                               + bufferSize),
                      std::streamsize(frame.data().size() - bufferSize));
        bufferSize += size_t(std::cin.gcount());

        if (bufferSize == frame.data().size()) {
            this->m_ipcBridge.write(deviceId, frame);
            bufferSize = 0;
        }
    } while (!std::cin.eof() && !exit);

    this->m_ipcBridge.deviceStop(deviceId);

    return 0;
}

int AkVCam::CmdParserPrivate::showControls(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    if (this->m_parseable) {
        for (auto &control: this->m_ipcBridge.controls(deviceId))
            std::cout << control.id << std::endl;
    } else {
        auto typeStr = typeStrMap();

        std::vector<std::string> table {
            "Control",
            "Description",
            "Type",
            "Minimum",
            "Maximum",
            "Step",
            "Default",
            "Value"
        };
        auto columns = table.size();

        for (auto &control: this->m_ipcBridge.controls(deviceId)) {
            table.push_back(control.id);
            table.push_back(control.description);
            table.push_back(typeStr[control.type]);
            table.push_back(std::to_string(control.minimum));
            table.push_back(std::to_string(control.maximum));
            table.push_back(std::to_string(control.step));
            table.push_back(std::to_string(control.defaultValue));
            table.push_back(std::to_string(control.value));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::readControl(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    for (auto &control: this->m_ipcBridge.controls(deviceId))
        if (control.id == args[2]) {
            if (flags.empty()) {
                std::cout << control.value << std::endl;
            } else {
                if (this->containsFlag(flags, "get-control", "-c")) {
                    auto typeStr = typeStrMap();
                    std::cout << control.description << std::endl;
                }
                if (this->containsFlag(flags, "get-control", "-t")) {
                    auto typeStr = typeStrMap();
                    std::cout << typeStr[control.type] << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-m")) {
                    std::cout << control.minimum << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-M")) {
                    std::cout << control.maximum << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-s")) {
                    std::cout << control.step << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-d")) {
                    std::cout << control.defaultValue << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-l")) {
                    for (size_t i = 0; i< control.menu.size(); i++)
                        if (this->m_parseable)
                            std::cout << control.menu[i] << std::endl;
                        else
                            std::cout << i << ": " << control.menu[i] << std::endl;
                }
            }

            return 0;
        }

    std::cerr << "'" << args[2] << "' control not available." << std::endl;

    return -1;
}

int AkVCam::CmdParserPrivate::writeControls(const StringMap &flags,
                                            const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -1;
    }

    std::map<std::string, int> controls;

    for (size_t i = 2; i < args.size(); i++) {
        if (args[i].find('=') == std::string::npos) {
            std::cerr << "Argumment "
                      << i
                      << " is not in the form KEY=VALUE."
                      << std::endl;

            return -1;
        }

        auto pair = splitOnce(args[i], "=");

        if (pair.first.empty()) {
            std::cerr << "Key for argumment "
                      << i
                      << " is emty."
                      << std::endl;

            return -1;
        }

        auto key = trimmed(pair.first);
        auto value = trimmed(pair.second);
        bool found = false;

        for (auto &control: this->m_ipcBridge.controls(deviceId))
                if (control.id == key) {
                    switch (control.type) {
                    case ControlTypeInteger: {
                        char *p = nullptr;
                        auto val = strtol(value.c_str(), &p, 10);

                        if (*p) {
                            std::cerr << "Value at argument "
                                      << i
                                      << " must be an integer."
                                      << std::endl;

                            return -1;
                        }

                        controls[key] = val;

                        break;
                    }

                    case ControlTypeBoolean: {
                        std::locale loc;
                        std::transform(value.begin(),
                                       value.end(),
                                       value.begin(),
                                       [&loc](char c) {
                            return std::tolower(c, loc);
                        });

                        if (value == "0" || value == "false") {
                            controls[key] = 0;
                        } else if (value == "1" || value == "true") {
                            controls[key] = 1;
                        } else {
                            std::cerr << "Value at argument "
                                      << i
                                      << " must be a boolean."
                                      << std::endl;

                            return -1;
                        }

                        break;
                    }

                    case ControlTypeMenu: {
                        char *p = nullptr;
                        auto val = strtoul(value.c_str(), &p, 10);

                        if (*p) {
                            auto it = std::find(control.menu.begin(),
                                                control.menu.end(),
                                                value);

                            if (it == control.menu.end()) {
                                std::cerr << "Value at argument "
                                          << i
                                          << " is not valid."
                                          << std::endl;

                                return -1;
                            }

                            controls[key] = int(it - control.menu.begin());
                        } else {
                            if (val >= control.menu.size()) {
                                std::cerr << "Value at argument "
                                          << i
                                          << " is out of range."
                                          << std::endl;

                                return -1;
                            }

                            controls[key] = int(val);
                        }

                        break;
                    }

                    default:
                        break;
                    }

                    found = true;

                    break;
                }

        if (!found) {
            std::cerr << "No such '"
                      << key
                      << "' control in argument "
                      << i
                      << "."
                      << std::endl;

            return -1;
        }
    }

    this->m_ipcBridge.setControls(deviceId, controls);

    return 0;
}

int AkVCam::CmdParserPrivate::picture(const AkVCam::StringMap &flags,
                                    const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    std::cout << this->m_ipcBridge.picture() << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setPicture(const AkVCam::StringMap &flags,
                                       const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    this->m_ipcBridge.setPicture(args[1]);

    return 0;
}

int AkVCam::CmdParserPrivate::logLevel(const AkVCam::StringMap &flags,
                                       const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    auto level = this->m_ipcBridge.logLevel();

    if (this->m_parseable)
        std::cout << level << std::endl;
    else
        std::cout << AkVCam::Logger::levelToString(level) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setLogLevel(const AkVCam::StringMap &flags,
                                          const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto levelStr = args[1];
    char *p = nullptr;
    auto level = strtol(levelStr.c_str(), &p, 10);

    if (*p)
        level = AkVCam::Logger::levelFromString(levelStr);

    this->m_ipcBridge.setLogLevel(level);

    return 0;
}

int AkVCam::CmdParserPrivate::showClients(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    auto clients = this->m_ipcBridge.clientsPids();

    if (clients.empty())
        return 0;

    if (this->m_parseable) {
        for (auto &pid: clients)
            std::cout << pid
                      << " "
                      << this->m_ipcBridge.clientExe(pid)
                      << std::endl;
    } else {
        std::vector<std::string> table {
            "Pid",
            "Executable"
        };
        auto columns = table.size();

        for (auto &pid: clients) {
            table.push_back(std::to_string(pid));
            table.push_back(this->m_ipcBridge.clientExe(pid));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

void AkVCam::CmdParserPrivate::loadGenerals(Settings &settings)
{
    settings.beginGroup("General");

    if (settings.contains("default_frame"))
        this->m_ipcBridge.setPicture(settings.value("default_frame"));

    if (settings.contains("loglevel")) {
        auto logLevel= settings.value("loglevel");
        char *p = nullptr;
        auto level = strtol(logLevel.c_str(), &p, 10);

        if (*p)
            level = AkVCam::Logger::levelFromString(logLevel);

        this->m_ipcBridge.setLogLevel(level);
    }

    settings.endGroup();
}

AkVCam::VideoFormatMatrix AkVCam::CmdParserPrivate::readFormats(Settings &settings)
{
    VideoFormatMatrix formatsMatrix;
    settings.beginGroup("Formats");
    auto nFormats = settings.beginArray("formats");

    for (size_t i = 0; i < nFormats; i++) {
        settings.setArrayIndex(i);
        formatsMatrix.push_back(this->readFormat(settings));
    }

    settings.endArray();
    settings.endGroup();

    return formatsMatrix;
}

std::vector<AkVCam::VideoFormat> AkVCam::CmdParserPrivate::readFormat(Settings &settings)
{
    std::vector<AkVCam::VideoFormat> formats;

    auto pixFormats = settings.valueList("format", ",");
    auto widths = settings.valueList("width", ",");
    auto heights = settings.valueList("height", ",");
    auto frameRates = settings.valueList("fps", ",");

    if (pixFormats.empty()
        || widths.empty()
        || heights.empty()
        || frameRates.empty()) {
        std::cerr << "Error reading formats." << std::endl;

        return {};
    }

    StringMatrix formatMatrix;
    formatMatrix.push_back(pixFormats);
    formatMatrix.push_back(widths);
    formatMatrix.push_back(heights);
    formatMatrix.push_back(frameRates);

    for (auto &format_list: this->matrixCombine(formatMatrix)) {
        auto pixFormat = VideoFormat::fourccFromString(format_list[0]);
        char *p = nullptr;
        auto width = strtol(format_list[1].c_str(), &p, 10);
        p = nullptr;
        auto height = strtol(format_list[2].c_str(), &p, 10);
        Fraction frame_rate(format_list[3]);
        VideoFormat format(pixFormat,
                           width,
                           height,
        {frame_rate});

        if (format.isValid())
            formats.push_back(format);
    }

    return formats;
}

AkVCam::StringMatrix AkVCam::CmdParserPrivate::matrixCombine(const StringMatrix &matrix)
{
    StringVector combined;
    StringMatrix combinations;
    this->matrixCombineP(matrix, 0, combined, combinations);

    return combinations;
}

/* A matrix is a list of lists where each element in the main list is a row,
 * and each element in a row is a column. We combine each element in a row with
 * each element in the next rows.
 */
void AkVCam::CmdParserPrivate::matrixCombineP(const StringMatrix &matrix,
                                              size_t index,
                                              StringVector combined,
                                              StringMatrix &combinations)
{
    if (index >= matrix.size()) {
        combinations.push_back(combined);

        return;
    }

    for (auto &data: matrix[index]) {
        auto combinedP1 = combined;
        combinedP1.push_back(data);
        this->matrixCombineP(matrix, index + 1, combinedP1, combinations);
    }
}

void AkVCam::CmdParserPrivate::createDevices(Settings &settings,
                                             const VideoFormatMatrix &availableFormats)
{
    auto devices = this->m_ipcBridge.devices();

    for (auto &device: devices)
        this->m_ipcBridge.removeDevice(device);

    settings.beginGroup("Cameras");
    size_t nCameras = settings.beginArray("cameras");

    for (size_t i = 0; i < nCameras; i++) {
        settings.setArrayIndex(i);
        this->createDevice(settings, availableFormats);
    }

    settings.endArray();
    settings.endGroup();
    this->m_ipcBridge.updateDevices();
}

void AkVCam::CmdParserPrivate::createDevice(Settings &settings,
                                            const VideoFormatMatrix &availableFormats)
{
    auto description = settings.value("description");

    if (description.empty()) {
        std::cerr << "Device description is empty" << std::endl;

        return;
    }

    auto formats = this->readDeviceFormats(settings, availableFormats);

    if (formats.empty()) {
        std::cerr << "Can't read device formats" << std::endl;

        return;
    }

    auto deviceId = this->m_ipcBridge.addDevice(description);
    auto supportedFormats = this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamTypeOutput);

    for (auto &format: formats) {
        auto it = std::find(supportedFormats.begin(),
                            supportedFormats.end(),
                            format.fourcc());

        if (it != supportedFormats.end())
            this->m_ipcBridge.addFormat(deviceId, format, -1);
    }
}

std::vector<AkVCam::VideoFormat> AkVCam::CmdParserPrivate::readDeviceFormats(Settings &settings,
                                                                             const VideoFormatMatrix &availableFormats)
{
    std::vector<AkVCam::VideoFormat> formats;
    auto formatsIndex = settings.valueList("formats", ",");

    for (auto &indexStr: formatsIndex) {
        char *p = nullptr;
        auto index = strtoul(indexStr.c_str(), &p, 10);

        if (*p)
            continue;

        index--;

        if (index >= availableFormats.size())
            continue;

        for (auto &format: availableFormats[index])
            formats.push_back(format);
    }

    return formats;
}

std::string AkVCam::operator *(const std::string &str, size_t n)
{
    std::stringstream ss;

    for (size_t i = 0; i < n; i++)
        ss << str;

    return ss.str();
}
