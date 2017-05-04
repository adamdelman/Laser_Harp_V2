import glob
import logging
import codecs
import fluidsynth.fluidsynth
import re
import serial
import os.path
import pyinotify
import time
from . import exceptions


class DeviceEventHandler(pyinotify.ProcessEvent):
    def my_init(self, **kwargs):
        self.note_controller_manager = kwargs['note_controller_manager']

    def _get_device_name(self, path):
        return os.path.split(path)[-1]

    def _is_usb_tty_device(self, path):
        logging.info("PATH IS %s", path)
        device_name_regex = "ttyUSB.*"
        match = re.match(device_name_regex, path)
        logging.info("MATCH IS %s", match)
        return match is not None

    def process_IN_CREATE(self, event):
        # Does nothing, just to demonstrate how stuffs could be done
        # after having processed statistics.
        if self._is_usb_tty_device(self._get_device_name(event.pathname)):
            time.sleep(0.5)
            self.note_controller_manager.register_note(event.pathname)

    def process_IN_DELETE(self, event):
        # Does nothing, just to demonstrate how stuffs could be done
        # after having processed statistics.
        if self._is_usb_tty_device(self._get_device_name(event.pathname)):
            self.note_controller_manager.unregister_note(event.pathname)


class NoteMessage:
    def __init__(self, note_active, octave_index):
        self.note_active = note_active
        self.octave = octave_index

    @classmethod
    def from_bytes(cls, byte_data):
        logging.info("Creating NoteMessage from data '%s'", byte_data)
        active = bool(int(byte_data[:2]))
        octave = int(byte_data[2:]) + 1
        return cls(active, octave)

    def get_midi_note(self):
        return self.octave * 12

    def __str__(self):
        return "NoteMessage(active={},octave={})".format(
            self.note_active, self.octave
        )


class NoteControllerManager:
    def __init__(self, soundfont_path):
        self.soundfont_path = soundfont_path
        self._note_index_to_controllers = {}
        self._com_port_to_note_index = {}
        self._synth = fluidsynth.fluidsynth.Synth()
        self._synth.start()
        self._soundfont_id = self._synth.sfload(self.soundfont_path)
        self._synth.program_select(0, self._soundfont_id, 0, 0)
        self._setup_note_controllers()

        self.device_watcher = pyinotify.WatchManager()
        self.device_notifier = pyinotify.ThreadedNotifier(self.device_watcher,
                                                          default_proc_fun=DeviceEventHandler(
                                                              note_controller_manager=self))
        self.device_watcher.add_watch('/dev/', pyinotify.IN_CREATE | pyinotify.IN_DELETE)
        self.device_notifier.start()

    @property
    def _com_ports(self):
        return glob.glob('/dev/ttyUSB*')

    def loop(self):
        while True:
            try:
                for note_controller in self._note_index_to_controllers.values():
                    note_controller.check_note()
            except KeyboardInterrupt as interrupt:
                self.device_notifier.stop()
                raise interrupt

    def _setup_note_controllers(self):
        for note_index, com_port in enumerate(self._com_ports):
            try:
                controller = NoteController(self, com_port)
            except serial.SerialException:
                continue
            else:
                self._note_index_to_controllers[note_index] = controller
                self._com_port_to_note_index[com_port] = note_index
                logging.info("Added note {note_index} for port {com_port}".format(note_index=note_index, com_port=com_port))

    def register_note(self, device_file):
        logging.info("Adding note for port {device_file}".format(device_file=device_file))
        if self._note_index_to_controllers:
            new_note_index = max((int(key) for key in self._note_index_to_controllers)) + 1
        else:
            new_note_index = 0
        try:
            controller = NoteController(self, device_file)
        except serial.SerialException:
            pass
        else:
            self._note_index_to_controllers[new_note_index] = controller
            self._com_port_to_note_index[device_file] = new_note_index
            logging.info("Added note {new_note_index} for port {device_file}".format(new_note_index=new_note_index,
                                                                              device_file=device_file))

    def unregister_note(self, device_file):
        logging.info("Removing note for port {device_file}".format(device_file=device_file))
        try:
            note_index = self._com_port_to_note_index[device_file]
        except KeyError:
            pass
        else:
            del self._note_index_to_controllers[note_index]
            del self._com_port_to_note_index[device_file]
            logging.info("Removed note {note_index} for port {device_file}".format(note_index=note_index,
                                                                            device_file=device_file))

    def note_on(self, note_number):
        logging.info("Note %s ON", note_number)
        self._synth.noteon(0, note_number, 127)

    def note_off(self, note_number):
        logging.info("Note %s OFF", note_number)
        self._synth.noteoff(0, note_number)


class NoteController:
    MSG_SIZE = 2

    def __init__(self, manager, com_port):
        """

        :type com_port: serial.Serial
        """
        logging.info("Creating note controller for device '%s'.", com_port)
        self._manager = manager
        self._baud_rate = 9600
        self.com_port = com_port
        self._serial = serial.Serial(com_port, self._baud_rate)
        self._last_activated_note = None

    def _note_on(self, note_number):
        self._manager.note_on(note_number)

    def _note_off(self, note_number):
        self._manager.note_off(note_number)

    def check_note(self):
        try:
            msg = self.get_message_from_com_port()
            logging.info(msg)
        except exceptions.NoMessageException:
            pass
        else:
            midi_note = msg.get_midi_note()
            if self._last_activated_note is not None and self._last_activated_note != midi_note:
                self._note_off(midi_note)
            if msg.note_active:
                self._note_on(midi_note)
            self._last_activated_note = midi_note

    def get_message_from_com_port(self):
        try:
            bytes_waiting = self._serial.inWaiting()
            if bytes_waiting >= self.MSG_SIZE:
                data = codecs.encode(self._serial.read(bytes_waiting), 'hex')
                return NoteMessage.from_bytes(data)
            else:
                raise exceptions.NoMessageException()
        except IOError:
            raise exceptions.NoMessageException()
