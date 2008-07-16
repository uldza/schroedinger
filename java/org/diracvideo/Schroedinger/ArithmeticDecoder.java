package org.diracvideo.Schroedinger;

class ArithmeticContext {
    int next, stat_range, n_bits, n_symbols, ones;
}

class ArithmeticDecoder {
    int code, range_size, cntr;
    byte shift;
    byte[] data;
    short probabilities[] = new short[70];
    int range[];
    int index, offset, carry, size;
    ArithmeticContext contexts[] = new ArithmeticContext[70];

    private short lut[] = new short[512]; /* look up table */

    public ArithmeticDecoder(Buffer b) {
	range =  new int[2];
	range[0] = 0;
	range[1] = 0xffff;
	range_size = 0xffff;
	index = b.b;
	size = b.e;
	data = b.d;
	code = ((size - index) > 0 ? data[0] : 0xff) << 8;
	code |= ((size - index) > 1 ? data[1] : 0xff);
	offset = 2;
    }

    
    public int decodeUint(int cont, int val) {
	int v = 1;
	while(decodeBit(cont) == 0) {
	    v = (v << 1) | decodeBit(val);
	}
	return v-1;
    }

    public int decodeSint(int cont, int val, int sign) {
	int v = decodeUint(cont, val);
	return (v == 0 || decodeBit(sign) == 0) ? v : -v;
    }

    public int decodeBit(int context) {
	return 1;
    }

}