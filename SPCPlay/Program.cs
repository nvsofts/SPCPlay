using System;
using System.IO;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Threading;

namespace SPCPlay
{
    class Program
    {
        public delegate int CallbackPortReadP(int address);
        public delegate void CallbackPortWriteP(int address, int data);

        [StructLayout(LayoutKind.Sequential)]
        public struct TRANSMITSPCEXCALLBACK
        {
            public uint cbSize;
            public uint transmitType;
            public uint bScript700;
            public CallbackPortReadP pCallbackRead;
            public CallbackPortWriteP pCallbackWrite;
        }

        [DllImport("snesapu.dll")]
        public static extern void LoadSPCFile(byte[] pFile);

        [DllImport("snesapu.dll")]
        public static extern uint TransmitSPCEx(ref TRANSMITSPCEXCALLBACK table);

        private static SerialPort serialPort;

        private static int APURead(int address)
        {
            int result = 0;

            Console.Write("[<--] Read $214{0}", address);

            Program.serialPort.Write(String.Format("R{0}\n", address));
            while (Program.serialPort.BytesToRead < 4) ;

            string readLine = Program.serialPort.ReadLine();

            result = Convert.ToInt32(readLine.Substring(1, 2), 16);

            Console.WriteLine(" -> 0x{0,0:X2}", result);

            return result;
        }

        private static void APUWrite(int address, int data)
        {
            Console.WriteLine("[-->] Write 0x{0,0:X2} to $214{1}", data, address);

            Program.serialPort.Write(String.Format("W{0}{1,0:X2}\n", address, data));
        }

        private static CallbackPortReadP dRead = new CallbackPortReadP(Program.APURead);
        private static CallbackPortWriteP dWrite = new CallbackPortWriteP(Program.APUWrite);

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: {0} <serial port> <path to .spc file>", Environment.GetCommandLineArgs()[0]);
                return;
            }

            byte[] spcData = File.ReadAllBytes(args[1]);

            // LoadSPCFile() requires 66048 bytes array at least
            if (spcData.Length < 66048)
            {
                Array.Resize(ref spcData, 66048);
            }

            Program.serialPort = new SerialPort(args[0], 115200, Parity.None, 8, StopBits.One);
            Program.serialPort.Open();

            // Send reset command
            Program.serialPort.Write("I\n");
            Thread.Sleep(50);

            var table = new TRANSMITSPCEXCALLBACK();

            table.cbSize = (uint)Marshal.SizeOf(table);
            table.transmitType = 0x03; // TRANSMIT_TYPE_CALLBACK
            table.bScript700 = 0;
            table.pCallbackRead = Program.dRead;
            table.pCallbackWrite = Program.dWrite;

            // Send data to SHVC-SOUND
            Program.LoadSPCFile(spcData);
            Program.TransmitSPCEx(ref table);

            Console.WriteLine("Enjoy!");
        }
    }
}
