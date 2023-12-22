package penguin.opencore.tester;

public class LeakMemory {
    private byte[] data = new byte[65536];

    public LeakMemory() {
        for (int i = 0; i < data.length; i++) {
            data[i] = (byte) 0xaa;
        }
    }
}
