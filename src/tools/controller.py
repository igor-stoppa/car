#!/usr/bin/python3

import _thread
import pygubu
import tkinter as tk
import serial
import time
import struct

class MyContainer(object):
    pass

class Communicator:
    def __init__(self):
        self.a = 0

    def openSerial(self, port):
        self.ser = serial.Serial(
            port=port,
            baudrate=34800,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS
        )
        return self.ser.isOpen()

    def serialRunCommand(self, command=''):
        output = ''
        returns_count = command.count('\n')
        self.ser.write(bytearray(command, 'ascii'))
        time.sleep(0.025 * returns_count)
        while self.ser.in_waiting > 0:
            output += self.ser.read().decode('utf-8')
        return output

    def serialRunCommandsSequence(self, sequence):
        io = []
        success = False
        for entry in sequence:
            command = list(entry.keys())[0]
            output = self.serialRunCommand(command)
            io.append({'command': command, 'output': output})
            if not output.strip(' \r\n').endswith(list(entry.values())[0]):
                print('---- Received ------\n')
                print(output.strip(' \r\n'))
                print('---- Expected Ending ------\n')
                print(list(entry.values())[0])
                print('----------\n')
                break
        else:
            success = True
        return success, io

    def initI2C(self):
        sequence = [
            {'\nmenu\n': '(1)>'},
            {'4\n': '(1)>'},
            {'2\n': '(1)>'},
            {'2\n': 'I2C>'},
        ]
        return self.serialRunCommandsSequence(sequence)


class Controller:
    def __init__(self):
        print("")

class UI:
    def __init__(self, master, communicator):
        self.master = master
        self.com = communicator
        self.builder = builder = pygubu.Builder()
        builder.add_from_file('controller.ui')
        self.mainframe = builder.get_object('MainFrame', master)
        builder.connect_callbacks(self)
        callbacks = {
            'on_BusPirateEnterI2CModeButton_clicked':
                self.on_BusPirateEnterI2CModeButton_clicked,
            'on_I2CMotorsApplyButton_clicked':
                self.on_I2CMotorsApplyButton_clicked,
            'on_I2CMotorsAcquireButton_clicked':
                self.on_I2CMotorsAcquireButton_clicked,
            'on_I2CMotorsHaltButton_clicked':
                self.on_I2CMotorsHaltButton_clicked,
            'on_I2CRawCommandTransmitButton_clicked':
                self.on_I2CRawCommandTransmitButton_clicked,
            'on_I2CRawCommandResetButton_clicked':
                self.on_I2CRawCommandResetButton_clicked,
            'on_I2CTachoCalibrateButton_clicked':
                self.on_I2CTachoCalibrateButton_clicked,
            'on_ExitButton_clicked':
                self.on_ExitButton_clicked,
            'on_I2CRawCommandHistory_clicked':
                self.on_I2CRawCommandHistory_clicked,
        }
        builder.connect_callbacks(callbacks)
        self.log = self.builder.get_object('I2CLogText', self.master)
        self.i2cirawaddress = \
            self.builder.get_object('I2CRawAddressEntry', self.master)
        self.rawcommand = \
        self.i2crawcommandhistory = \
            self.builder.get_object('I2CRawCommandsHistoryListbox',
                                    self.master)
        self.rawcommand = \
            self.builder.get_object('I2CRawCommandEntry', self.master)

        self.motorsnotebook = \
            self.builder.get_object('MotorsNotebook', self.master)

        self.rawmotorsselection = \
            self.builder.get_variable("RawMotorsSelection")

        self.i2cmotorsaddress = \
            self.builder.get_object('I2CMotorsAddressEntry', self.master)

        self.simplerawmotorslist = [
            "LeftFrontRawMotor", "LeftRearRawMotor",
            "RightFrontRawMotor", "RightRearRawMotor",
        ]

        self.siderawmotorslist = [
            "AllRawMotors", "LeftSideRawMotors", "RightSideRawMotors",
        ]

        self.status_dict = {
            "idle": "0", "backward": "1", "forward": "2", "locked": "3",
        }

        self.rawmotorslist = \
            ["AllRawMotors"] + \
            self.siderawmotorslist + \
            self.simplerawmotorslist

        self.motor = {}
        for entry in self.rawmotorslist:
            self.motor[entry] = {}
            for parameter in {"Status", "PWM"}:
                self.motor[entry][parameter.lower()] = \
                    self.builder.get_variable(entry + parameter + "Output")

        for motor in list(self.motor.keys()):
            self.motor[motor]["status"].set("idle")
            self.motor[motor]["pwm"].set(0)

    def I2CCommunicate(self, message, returnbytes=0):
        sequence = [
            {(message + '\n'): 'I2C>'},
        ]
        success, io = self.com.serialRunCommandsSequence(sequence)
        self.logI2CIO(io)
        return success, io


    def I2CReadCommand(self, address, command, data):
        string = "[ " + \
            "0x" + format(int(address, 0) * 2, "x") + \
            " " + \
            command + \
            " [ " + \
            "0x" + format(int(address, 0) * 2 + 1, "x") + \
            " r" * int(str(data), 0) + \
            " ]"
        return self.I2CCommunicate(message=string)

    def I2CWriteCommand(self, address, command, data):
        string = "[ " + \
            "0x" + format(int(address, 0) * 2, "x") + \
            " " + \
            command + \
            " " + \
            " ".join(["0x" + format(int(d, 0), "x") for d in data]) + \
            " ]"
        return self.I2CCommunicate(message=string)


    def I2CApplyMotorsCommand(self, command,
                              motor="NoRawMotors", magnitude="0"):
        motors_dict = {
            "LeftFrontRawMotor": "3", "LeftRearRawMotor": "0",
            "RightFrontRawMotor": "2", "RightRearRawMotor": "1",
            "AllRawMotors": "4", "NoRawMotors": "0",
        }
        read_commands = {
            "GetIntensity":   {"value": "1", "bytes_read": 1},
            "GetDirection":   {"value": "2", "bytes_read": 1},
            "GetCalibPeriod": {"value": "4", "bytes_read": 2},
            "GetAvgPeriod":   {"value": "5", "bytes_read": 2},
            "GetCount":       {"value": "6", "bytes_read": 2},
            "GetM":           {"value": "7", "bytes_read": 4},
            "GetQ":           {"value": "8", "bytes_read": 4},
        }
        write_commands = {
            "SetIntensity":     {"value": "1", "bytes_read": 0},
            "SetDirection":     {"value": "2", "bytes_read": 0},
            "SetAction":        {"value": "3", "bytes_read": 0},
            "ResetCalibPeriod": {"value": "4", "bytes_read": 1},
            "ResetAvgPeriod":   {"value": "5", "bytes_read": 1},
            "ResetCount":       {"value": "6", "bytes_read": 1},
        }
        retvals = []
        if command in read_commands:
            success, io = self.I2CReadCommand(
                address=self.i2cmotorsaddress.get(),
                command="0x" + read_commands[command]["value"] + motors_dict[motor],
                data=read_commands[command]["bytes_read"],
            )
            if success:
                for message in io:
                    for row in message["output"].split("\r\n"):
                        if "READ" in row:
                            for element in row.split():
                                if '0x' in element:
                                    retvals.append(element)
        elif command in write_commands:
            if magnitude in list(self.status_dict.keys()):
                magnitude = "0x" + self.status_dict[magnitude]
            else:
                magnitude = "0x" + format(int(magnitude, 0), "x")
            success, io = self.I2CWriteCommand(
                address=self.i2cmotorsaddress.get(),
                command="0x" + write_commands[command]["value"] + motors_dict[motor],
                data=[magnitude],
            )
        else:
            print("Error command: " + command)
            return False, retvals
        return success, retvals

    def I2CRawMotorsApply(self, motor, status, pwm):
        self.I2CApplyMotorsCommand(
            command="SetIntensity",
            motor=motor,
            magnitude=pwm,
        )
        self.I2CApplyMotorsCommand(
            command="SetDirection",
            motor=motor,
            magnitude=status,
        )


    def I2CApplyAllRawMotors(self):
        self.I2CRawMotorsApply(
            motor="AllRawMotors",
            status=self.motor["AllRawMotors"]["status"].get(),
            pwm=str(self.motor["AllRawMotors"]["pwm"].get()),
        )

    def I2CApplySidesRawMotors(self):
        for side in ["Left", "Right"]:
            for x in ["Front", "Rear"]:
                self.I2CRawMotorsApply(
                    motor=side + x + "RawMotor",
                    status=self.motor[side + "Side" + "RawMotors"]["status"].get(),
                    pwm=str(self.motor[side + "Side" + "RawMotors"]["pwm"].get()),
                )

    def I2CApplyIndividualsRawMotors(self):
        for side in ["Left", "Right"]:
            for x in ["Front", "Rear"]:
                self.I2CRawMotorsApply(
                    motor=side + x + "RawMotor",
                    status=self.motor[side + x + "RawMotor"]["status"].get(),
                    pwm=str(self.motor[side + x + "RawMotor"]["pwm"].get()),
                )

    def I2CMotorsApply(self):
        active_table = {
            "Raw": self.rawmotorsselection,
        }
        apply_table = {
            "Raw": {
                "AllRawMotors": self.I2CApplyAllRawMotors,
                "SidesRawMotors": self.I2CApplySidesRawMotors,
                "IndividualRawMotors": self.I2CApplyIndividualsRawMotors,
            },
        }
        activemotorstab = \
            self.motorsnotebook.tab(self.motorsnotebook.select(), "text")
        activeselection = active_table[activemotorstab].get()
        apply_table[activemotorstab][activeselection]()

    def I2CMotorAcquireRaw(self, sideLR, sideFR):
        for parameter in ["Direction", "Intensity"]:
            success, retvals = self.I2CApplyMotorsCommand(
                                   command="Get" + parameter,
                                   motor=sideLR + sideFR + "RawMotor",
                               )
            if success:
                item_name = sideLR + sideFR + "RawMotor" + parameter + "Input"
                item = self.builder.get_variable(item_name)
                value = " ".join(retvals)
                if parameter is "Direction":
                    value = list(self.status_dict) \
                                [list(self.status_dict.values()).index(str(int(value, 16)))]
                item.set(value)

    def I2CMotorsAcquireRaw(self):
        for sideLR in ["Left", "Right"]:
            for sideFR in ["Front", "Rear"]:
                self.I2CMotorAcquireRaw(sideLR=sideLR, sideFR=sideFR)

    def I2CMotorAcquireTacho(self, sideLR, sideFR):
        for parameter in ["CalibPeriod", "AvgPeriod", "Count", "M", "Q"]:
            success, retvals = self.I2CApplyMotorsCommand(
                                   command="Get" + parameter,
                                   motor=sideLR + sideFR + "RawMotor",
                               )
            if success:
                item_name = sideLR + sideFR + "TachoMotor" + parameter + "Input"
                item = self.builder.get_variable(item_name)
                retvals.reverse()
                value = "".join([retv[len("0x"):] for retv in retvals])
                if parameter in ["M", "Q"]:
                    value = "{:.2f}".format(struct.unpack('!f', bytes.fromhex(value))[0])
                else:
                  value = "0x" + value
                item.set(value)

    def I2CMotorsAcquireTacho(self):
        for sideLR in ["Left", "Right"]:
            for sideFR in ["Front", "Rear"]:
                self.I2CMotorAcquireTacho(sideLR=sideLR, sideFR=sideFR)

    def I2CMotorsAcquire(self):
        self.I2CMotorsAcquireRaw()
        self.I2CMotorsAcquireTacho()

    def I2CMotorsReset(self):
        self.I2CRawMotorsApply(
            motor="AllRawMotors",
            status="idle",
            pwm="0",
        )

    def logSerialMessage(self, message):
        self.log['state'] = 'normal'
        self.log.insert(tk.END, message)
        self.log.see(tk.END)
        self.log['state'] = 'disabled'

    def logI2CIO(self, io):
        for entry in io:
            self.logSerialMessage(message=entry['output'])

    def enterBusPirateI2CMode(self):
        port = self.builder.get_object('PathSerialPortEntry', self.master).get()
        if self.com.openSerial(port=port):
            for item_name in ['I2CRawCommandTransmitButton']:
                item = self.builder.get_object(item_name, self.master)
                item['state'] = 'normal'
        success, io = self.com.initI2C()
        self.logI2CIO(io)

    def calibrateTacho(self):
        success, retvals = self.I2CApplyMotorsCommand(
                               command="SetAction",
                               magnitude="1",
                           )
        self.I2CMotorsAcquireTacho()

    def fetchI2CRawCommandFromHistory(self, evt):
        w = evt.widget
        index = int(w.curselection()[0])
        rawcommand = w.get(index)
        self.setI2CRawCommand(rawcommand=rawcommand)

    def addI2CRawCommandToHistory(self, rawcommand):
        rawcommands = self.i2crawcommandhistory.get(0, tk.END)
        if rawcommand not in rawcommands:
            self.i2crawcommandhistory.insert(tk.END, rawcommand)

    def setI2CRawCommand(self, rawcommand=''):
        self.rawcommand.delete(0, tk.END)
        if rawcommand is '':
            self.rawcommand.insert(0, '[ ' + self.i2caddress.get() + ' ]')
        else:
            self.rawcommand.insert(0, rawcommand)

    def transmitI2CRawCommand(self):
        rawcommand = self.rawcommand.get()
        self.addI2CRawCommandToHistory(rawcommand=rawcommand)
        sequence = [
            {(rawcommand + '\n'): ''},
        ]
        success, io = self.com.serialRunCommandsSequence(sequence)
        self.logI2CIO(io)

    def selectMotorsControlMode(self):
        self.builder.get_variable("MotorsSelection").get()

    def on_BusPirateEnterI2CModeButton_clicked(self):
        self.enterBusPirateI2CMode()

    def on_I2CMotorsApplyButton_clicked(self):
        self.I2CMotorsApply()

    def on_I2CMotorsAcquireButton_clicked(self):
        self.I2CMotorsAcquire()

    def on_I2CMotorsHaltButton_clicked(self):
        self.I2CMotorsReset()

    def on_I2CRawCommandTransmitButton_clicked(self):
        self.transmitI2CRawCommand()

    def on_I2CRawCommandHistory_clicked(self, evt):
        self.fetchI2CRawCommandFromHistory(evt)

    def on_I2CRawCommandResetButton_clicked(self):
        self.setI2CRawCommand()

    def on_I2CTachoCalibrateButton_clicked(self):
        self.calibrateTacho()

    def on_ExitButton_clicked(self):
        exit()

if __name__ == '__main__':
    root = tk.Tk()
    app = UI(root, Communicator())
    root.mainloop()
