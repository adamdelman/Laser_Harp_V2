import glob
import logging
import codecs
import fluidsynth.fluidsynth
import serial

from . import exceptions


class NoteMessage:
    def __init__(self, note_index, note_active, octave_index):
        self.note_index = note_index
        self.note_active = note_active
        self.octave_index = octave_index

    @classmethod
    def from_bytes(cls, note_index, byte_data):
        print(byte_data[:1])
        print(byte_data[2:])
        active = bool(int(byte_data[:2]))
        octave = int(byte_data[2:])
        return cls(note_index, active, octave)

    def get_midi_note(self):
        return self.octave_index * 12 + self.note_index

    def __str__(self):
        return "NoteMessage(note_index={},active={},octave={})".format(
            self.note_index, self.note_active, self.octave_index
        )


class NoteControllerManager:
    def __init__(self, soundfont_path):
        self.soundfont_path = soundfont_path
        self._note_controllers = []
        self._synth = fluidsynth.fluidsynth.Synth()
        self._synth.start()
        self._soundfont_id = self._synth.sfload(self.soundfont_path)
        self._synth.program_select(0, self._soundfont_id, 0, 0)
        self._setup_note_controllers()

    def loop(self):
        while True:
            for note_controller in self._note_controllers:
                note_controller.check_notes()

    @property
    def _com_ports(self):
        return glob.glob('/dev/ttyUSB*')

    def _setup_note_controllers(self):
        for note_index, com_port in enumerate(self._com_ports):
            print("Added note {note_index} for port {com_port}".format(note_index=note_index, com_port=com_port))
            self._note_controllers.append(NoteController(self._synth, note_index, com_port))


class NoteController:
    MSG_SIZE = 2

    def __init__(self, synth, note_index, com_port):
        """

        :type note_index: int
        :type com_port: serial.Serial
        """
        self._synth = synth
        self._baud_rate = 9600
        self.note_index = note_index
        self.com_port = com_port
        self._serial = serial.Serial(com_port, self._baud_rate)
        self._last_activated_note = None

    def _note_on(self, note_number):
        logging.info("Note %s ON", note_number)
        self._synth.noteon(0, note_number, 127)

    def _note_off(self, note_number):
        logging.info("Note %s OFF", note_number)
        self._synth.noteoff(0, note_number)

    def check_notes(self):
        try:
            msg = self.get_message_from_com_port()
            print(msg)
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
        bytes_waiting = self._serial.inWaiting()
        if bytes_waiting >= self.MSG_SIZE:
            data = codecs.encode(self._serial.read(bytes_waiting), 'hex')
            logging.info("DATA is '%s'", data)
            return NoteMessage.from_bytes(self.note_index, data)
        else:
            raise exceptions.NoMessageException()
