package org.diracvideo.Schroedinger;

/** Wavelet:
 *
 * The class for doing wavelet transformations.
 * It currently only provides inverse transformations. */

public class Wavelet {
    /** inverse:
     * Inverse is the only method users should ever call, except possibly 
     * for interleave, which interleaves four subbands so that it is ready
     * to be used for inverse(). The actual picture decoding sequence never
     * calles interleave() though.  
     *
     * @param data   a short[] array containing the frame
     * @param width  width of the frame
     * @param depth  transform depth     */
    public void inverse(short data[], int w, int depth) {
	/* data is assumed to be preinterleaved */
	for(int s = (1 << (depth - 1)); s > 0; s >>= 1) {
	    for(int x = 0; x < w; x += s) {
		synthesize(data,s*w,x,data.length); /* a column */
	    }
	    for(int y = 0; y < data.length; y += w*s) {
		synthesize(data,s,y, y + w); /* a row */
	    }
	    filtershift(data, w, s);
	}
    }

    public void filtershift(short data[], int w, int s ) {
	for(int y = 0; y < data.length; y += s*w) {
	    for(int x = 0; x < w; x += s) {
		data[y+x] = (short)((data[y+x]+1)>>1);
	    }
	}
    }

    public void inverse(Block block, int depth) {
	inverse(block.d, block.s.width, depth);
    }

    /** synthesize:
     * This method is public for testing purposes only. 
     * It is identity (i.e. does nothing) on the 'Wavelet' 
     * wavelet. It is implemented for other classes though.
     *
     * @param d   short[] data array
     * @param s   stride
     * @param b   begin
     * @param e   end */

    public void synthesize(short d[], int s, int b, int e) {  }

    /** interleave interleaves four subbands.
     * 
     * @param ll array containing the subband data (like hl, lh, hh)
     * @param width: the width of each subband 
     * @return the new subband, 4 times the size of each individual argument 
     * @see the dirac specification section 15.6.1 */

    public short[] interleave(short ll[], short hl[], 
			      short lh[], short hh[], int width) {
	short o[] = new short[ll.length*4];
	int height = ll.length / width;
	for(int y = 0; y < height; y++) {
	    for(int x = 0; x < width; x++) {
		int pos = (x + y*width);
		int outpos = 2*x + 4*y*width;
		o[outpos] = ll[pos];
		o[outpos+1] = hl[pos];
		o[outpos+2*width] = lh[pos];
		o[outpos+2*width+1] = hh[pos];
	    }
	}
	return o;
    }

}


class LeGall5_3 extends Wavelet {
    public void synthesize(short d[], int s, int b, int e) {
	for(int i = b; i < e; i += 2*s) {
	    if(i - s < b) {
		d[i] -= (d[i+s] + 1) >> 1;
	    } else {
		d[i] -= (d[i-s] + d[i+s] + 2) >> 2;
	    }
	}
	for(int i = b + s; i < e; i += 2*s) {
	    if(i + s >= e) {
		d[i] += d[i-s];
	    } else {
		d[i] += (d[i-s] + d[i+s] + 1) >> 1;
	    }
	}
    }
}

class DeslauriesDebuc9_7 extends Wavelet {
    public void	synthesize(short d[], int s, int b, int e) {
	for(int i = b; i < e; i += 2*s) {
	    if(i - s < b) {
		d[i] -= (d[i+s] + 1) >> 1;
	    } else {
		d[i] -= (d[i-s] + d[i+s] + 2) >> 2;
	    }
	}
	for(int i = b + s; i < e; i+= 2*s) {
	    if(i - 3*s >= b) {
		if(i+3*s < e)
		    d[i] += (9*d[i-s] + 9*d[i+s] - d[i-3*s] - d[i+3*s] + 8) >> 4;
		else if(i + s < e) 
		    d[i] += (9*d[i-s] + 8*d[i+s] - d[i-3*s] + 8) >> 4;
		else
		    d[i] += (17*d[i-s] - d[i-3*s] + 8) >> 4;
	    } else if(i - s >= b) {
		if(i + 3*s < e) {
		    d[i] += (8*d[i-s] + 9*d[i+s] - d[i+3*s] + 8) >> 4;
		} else if(i + s < e) {
		    d[i] += (8*d[i-s] + 8*d[i+s] + 8) >> 4;
		} else {
		    d[i] += (16*d[i-s] + 8) >> 4;
		}
	    } else {
		if(i + 3*s < e) {
		    d[i] += (17*d[i+s] - d[i+3*s] + 8) >> 4;
		} else if(i + s < e) {
		    d[i] += (16*d[i+s] + 8) >> 4;
		}
	    }
	} 
    }
}

class DeslauriesDebuc13_7 extends Wavelet {

    public void	synthesize(short d[], int s, int b, int e) {
	for(int i = b; i < e; i += 2*s) {
	    if(i - 3*s >= b) {
		if(i+3*s < e)
		    d[i] -= (9*d[i-s] + 9*d[i+s] - d[i-3*s] - d[i+3*s] + 16) >> 5;
		else if(i + s < e) 
		    d[i] -= (9*d[i-s] + 8*d[i+s] - d[i-3*s] + 16) >> 5;
		else
		    d[i] -= (17*d[i-s] - d[i-3*s] + 16) >> 5;
	    } else if(i - s >= b) {
		if(i + 3*s < e) {
		    d[i] -= (8*d[i-s] + 9*d[i+s] - d[i+3*s] + 16) >> 5;
		} else if(i + s < e) {
		    d[i] -= (8*d[i-s] + 8*d[i+s] + 16) >> 5;
		} else {
		    d[i] -= (16*d[i-s] + 16) >> 5;
		}
	    } else {
		if(i + 3*s < e) {
		    d[i] -= (17*d[i+s] - d[i+3*s] + 16) >> 5;
		} else if(i + s < e) {
		    d[i] -= (16*d[i+s] + 16) >> 5;
		}
	    }
	}
	for(int i = b + s; i < e; i+= 2*s) {
	    if(i - 3*s >= b) {
		if(i+3*s < e)
		    d[i] += (9*d[i-s] + 9*d[i+s] - d[i-3*s] - d[i+3*s] + 8) >> 4;
		else if(i + s < e) 
		    d[i] += (9*d[i-s] + 8*d[i+s] - d[i-3*s] + 8) >> 4;
		else
		    d[i] += (17*d[i-s] - d[i-3*s] + 8) >> 4;
	    } else if(i - s >= b) {
		if(i + 3*s < e) {
		    d[i] += (8*d[i-s] + 9*d[i+s] - d[i+3*s] + 8) >> 4;
		} else if(i + s < e) {
		    d[i] += (8*d[i-s] + 8*d[i+s] + 8) >> 4;
		} else {
		    d[i] += (16*d[i-s] + 8) >> 4;
		}
	    } else {
		if(i + 3*s < e) {
		    d[i] += (17*d[i+s] - d[i+3*s] + 8) >> 4;
		} else if(i + s < e) {
		    d[i] += (16*d[i+s] + 8) >> 4;
		}
	    }
	} 
    }

}

class HaarNoShift extends HaarSingleShift {
    public void filtershift(short d[], int w, int s) {}
}

class HaarSingleShift extends Wavelet {
    public void synthesize(short d[], int s, int b, int e) {
	for(int i = b; i < e - s; i += 2*s) {
	    d[i] -= (d[i+s] + 1) >> 1;
	}
	for(int i = b + s; i < e; i += 2*s) {
	    d[i] += d[i-s];
	}
    }
}

class Fidelity extends Wavelet {
    public void synthesize(short d[], int s, int b, int e) {
	for(int i = b + s; i < e; i += 2*s) {
	    int sum = 0;
	    if(i - 7*s >= b) {
		sum = -2*d[i-7*s] + 10*d[i-5*s] - 25*d[i-3*s] + 81*d[i-s];
	    } else if(i - 5*s >= b) {
		sum = 8*d[i-5*s] - 25*d[i-3*s] + 81*d[i-s];
	    } else if(i - 3*s >= b) {
		sum = -17*d[i-3*s] + 81*d[i-s];
	    } else if(i - s >= b) {
		sum = 64*d[i-s];
	    }
	    if(i + 7*s < e) {
		sum += -2*d[i+7*s] + 10*d[i+5*s] - 25*d[i+3*s] + 81*d[i+s];
	    } else if(i + 5*s < e) {
		sum += 8*d[i+5*s] - 25*d[i+3*s] + 81*d[i+s];
	    } else if(i + 3*s < e) {
		sum += -17*d[i+3*s] + 81*d[i+s];
	    } else if(i + s < e) {
		sum += 64*d[i+s];
	    }
	    d[i] += (sum + 128) >> 8;
	}
	for(int i = b; i < e; i += 2*s) {
	    int sum = 0;
	    if(i - 7*s >= b) {
		sum = -8*d[i-7*s] + 21*d[i-5*s] - 46*d[i-3*s]  + 161*d[i-s];
	    } else if(i - 5*s >= b) {
		sum = 13*d[i-5*s] - 46*d[i-3*s]  + 161*d[i-s];
	    } else if(i - 3*s >= b) {
		sum = -33*d[i-3*s]  + 161*d[i-s];
	    } else if(i - s >= b) {
		sum = 128*d[i-s];
	    }
	    if(i + 7*s < e) {
		sum += -8*d[i+7*s] + 21*d[i+5*s] - 46*d[i+3*s]  + 161*d[i+s];
	    } else if(i + 5*s < e) {
		sum += 13*d[i+5*s] - 46*d[i+3*s]  + 161*d[i+s];
	    } else if(i + 3*s < e) { 
		sum += 33*d[i+3*s]  + 161*d[i+s];
	    } else if(i + s < e) {
		sum += 128*d[i+s];
	    }
	    d[i] -= (sum + 128) >> 8;
	}
    }
    
    public void filtershift(short d[], int w, int s) {}
}

class Daubechies9_7 extends Wavelet {
    public void synthesize(short d[], int s, int b, int e) {
	for(int i = b; i < e; i += 2*s) {
	    if(i - s < b) {
		d[i] -= (3634*d[i+s] + 2048) >> 12;
	    } else {
		d[i] -= (1817*d[i-s] + 1817*d[i+s] + 2048) >> 12;
	    }
	}
	for(int i = b + s; i < e; i += 2*s) {
	    if(i + s >= e) {
		d[i] -= (7232*d[i-s] + 2048) >> 12;
	    } else {
		d[i] -= (3616*d[i-s] + 3616*d[i+s] + 2048) >> 12;
	    }
	}
	for(int i = b; i < e; i += 2*s) {
	    if(i - s < b) {
		d[i] += (434*d[i+s] + 2048) >> 12;
	    } else {
		d[i] += (217*d[i-s] + 217*d[i+s] + 2048) >> 12;
	    }
	}

	for(int i = b + s; i < e; i += 2*s) {
	    if(i + s >= e) {
		d[i] += (12996*d[i-s] + 2048) >> 12;
	    } else {
		d[i] += (6497*d[i-s] + 6497*d[i+s] + 2048) >> 12;
	    }
	}
    }

}
