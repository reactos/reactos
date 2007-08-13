// DeflaterHuffman.cs
// Copyright (C) 2001 Mike Krueger
//
// This file was translated from java, it was part of the GNU Classpath
// Copyright (C) 2001 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Linking this library statically or dynamically with other modules is
// making a combined work based on this library.  Thus, the terms and
// conditions of the GNU General Public License cover the whole
// combination.
// 
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent
// modules, and to copy and distribute the resulting executable under
// terms of your choice, provided that you also meet, for each linked
// independent module, the terms and conditions of the license of that
// module.  An independent module is a module which is not derived from
// or based on this library.  If you modify this library, you may extend
// this exception to your version of the library, but you are not
// obligated to do so.  If you do not wish to do so, delete this
// exception statement from your version.

using System;

namespace ICSharpCode.SharpZipLib.Zip.Compression 
{
	
	/// <summary>
	/// This is the DeflaterHuffman class.
	/// 
	/// This class is <i>not</i> thread safe.  This is inherent in the API, due
	/// to the split of deflate and setInput.
	/// 
	/// author of the original java version : Jochen Hoenicke
	/// </summary>
	public class DeflaterHuffman
	{
		private static  int BUFSIZE = 1 << (DeflaterConstants.DEFAULT_MEM_LEVEL + 6);
		private static  int LITERAL_NUM = 286;
		private static  int DIST_NUM = 30;
		private static  int BITLEN_NUM = 19;
		private static  int REP_3_6    = 16;
		private static  int REP_3_10   = 17;
		private static  int REP_11_138 = 18;
		private static  int EOF_SYMBOL = 256;
		private static  int[] BL_ORDER = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
		
		private static byte[] bit4Reverse = {
			0,
			8,
			4,
			12,
			2,
			10,
			6,
			14,
			1,
			9,
			5,
			13,
			3,
			11,
			7,
			15
		};
		
		public class Tree 
		{
			public short[] freqs;
			public byte[]  length;
			public int     minNumCodes, numCodes;
			
			short[] codes;
			int[]   bl_counts;
			int     maxLength;
			DeflaterHuffman dh;
			
			public Tree(DeflaterHuffman dh, int elems, int minCodes, int maxLength) 
			{
				this.dh =  dh;
				this.minNumCodes = minCodes;
				this.maxLength  = maxLength;
				freqs  = new short[elems];
				bl_counts = new int[maxLength];
			}
			
			public void Reset() 
			{
				for (int i = 0; i < freqs.Length; i++) {
					freqs[i] = 0;
				}
				codes = null;
				length = null;
			}
			
			public void WriteSymbol(int code)
			{
				//				if (DeflaterConstants.DEBUGGING) {
				//					freqs[code]--;
				//					//  	  Console.Write("writeSymbol("+freqs.length+","+code+"): ");
				//				}
				dh.pending.WriteBits(codes[code] & 0xffff, length[code]);
			}
			
			public void CheckEmpty()
			{
				bool empty = true;
				for (int i = 0; i < freqs.Length; i++) {
					if (freqs[i] != 0) {
						//Console.WriteLine("freqs["+i+"] == "+freqs[i]);
						empty = false;
					}
				}
				if (!empty) {
					throw new Exception();
				}
				//Console.WriteLine("checkEmpty suceeded!");
			}
			
			public void SetStaticCodes(short[] stCodes, byte[] stLength)
			{
				codes = stCodes;
				length = stLength;
			}
			
			public void BuildCodes() 
			{
				int numSymbols = freqs.Length;
				int[] nextCode = new int[maxLength];
				int code = 0;
				codes = new short[freqs.Length];
				
				//				if (DeflaterConstants.DEBUGGING) {
				//					//Console.WriteLine("buildCodes: "+freqs.Length);
				//				}
				
				for (int bits = 0; bits < maxLength; bits++) {
					nextCode[bits] = code;
					code += bl_counts[bits] << (15 - bits);
					//					if (DeflaterConstants.DEBUGGING) {
					//						//Console.WriteLine("bits: "+(bits+1)+" count: "+bl_counts[bits]
					//						                  +" nextCode: "+code); // HACK : Integer.toHexString(
					//					}
				}
				if (DeflaterConstants.DEBUGGING && code != 65536) {
					throw new Exception("Inconsistent bl_counts!");
				}
				
				for (int i=0; i < numCodes; i++) {
					int bits = length[i];
					if (bits > 0) {
						//						if (DeflaterConstants.DEBUGGING) {
						//								//Console.WriteLine("codes["+i+"] = rev(" + nextCode[bits-1]+")," // HACK : Integer.toHexString(
						//								                  +bits);
						//						}
						codes[i] = BitReverse(nextCode[bits-1]);
						nextCode[bits-1] += 1 << (16 - bits);
					}
				}
			}
			
			void BuildLength(int[] childs)
			{
				this.length = new byte [freqs.Length];
				int numNodes = childs.Length / 2;
				int numLeafs = (numNodes + 1) / 2;
				int overflow = 0;
				
				for (int i = 0; i < maxLength; i++) {
					bl_counts[i] = 0;
				}
				
				/* First calculate optimal bit lengths */
				int[] lengths = new int[numNodes];
				lengths[numNodes-1] = 0;
				
				for (int i = numNodes - 1; i >= 0; i--) {
					if (childs[2*i+1] != -1) {
						int bitLength = lengths[i] + 1;
						if (bitLength > maxLength) {
							bitLength = maxLength;
							overflow++;
						}
						lengths[childs[2*i]] = lengths[childs[2*i+1]] = bitLength;
					} else {
						/* A leaf node */
						int bitLength = lengths[i];
						bl_counts[bitLength - 1]++;
						this.length[childs[2*i]] = (byte) lengths[i];
					}
				}
				
				//				if (DeflaterConstants.DEBUGGING) {
				//					//Console.WriteLine("Tree "+freqs.Length+" lengths:");
				//					for (int i=0; i < numLeafs; i++) {
				//						//Console.WriteLine("Node "+childs[2*i]+" freq: "+freqs[childs[2*i]]
				//						                  + " len: "+length[childs[2*i]]);
				//					}
				//				}
				
				if (overflow == 0) {
					return;
				}
				
				int incrBitLen = maxLength - 1;
				do {
					/* Find the first bit length which could increase: */
					while (bl_counts[--incrBitLen] == 0)
						;
					
					/* Move this node one down and remove a corresponding
					* amount of overflow nodes.
					*/
					do {
						bl_counts[incrBitLen]--;
						bl_counts[++incrBitLen]++;
						overflow -= 1 << (maxLength - 1 - incrBitLen);
					} while (overflow > 0 && incrBitLen < maxLength - 1);
				} while (overflow > 0);
				
				/* We may have overshot above.  Move some nodes from maxLength to
				* maxLength-1 in that case.
				*/
				bl_counts[maxLength-1] += overflow;
				bl_counts[maxLength-2] -= overflow;
				
				/* Now recompute all bit lengths, scanning in increasing
				* frequency.  It is simpler to reconstruct all lengths instead of
				* fixing only the wrong ones. This idea is taken from 'ar'
				* written by Haruhiko Okumura.
				*
				* The nodes were inserted with decreasing frequency into the childs
				* array.
				*/
				int nodePtr = 2 * numLeafs;
				for (int bits = maxLength; bits != 0; bits--) {
					int n = bl_counts[bits-1];
					while (n > 0) {
						int childPtr = 2*childs[nodePtr++];
						if (childs[childPtr + 1] == -1) {
							/* We found another leaf */
							length[childs[childPtr]] = (byte) bits;
							n--;
						}
					}
				}
				//				if (DeflaterConstants.DEBUGGING) {
				//					//Console.WriteLine("*** After overflow elimination. ***");
				//					for (int i=0; i < numLeafs; i++) {
				//						//Console.WriteLine("Node "+childs[2*i]+" freq: "+freqs[childs[2*i]]
				//						                  + " len: "+length[childs[2*i]]);
				//					}
				//				}
			}
			
			public void BuildTree()
			{
				int numSymbols = freqs.Length;
				
				/* heap is a priority queue, sorted by frequency, least frequent
				* nodes first.  The heap is a binary tree, with the property, that
				* the parent node is smaller than both child nodes.  This assures
				* that the smallest node is the first parent.
				*
				* The binary tree is encoded in an array:  0 is root node and
				* the nodes 2*n+1, 2*n+2 are the child nodes of node n.
				*/
				int[] heap = new int[numSymbols];
				int heapLen = 0;
				int maxCode = 0;
				for (int n = 0; n < numSymbols; n++) {
					int freq = freqs[n];
					if (freq != 0) {
						/* Insert n into heap */
						int pos = heapLen++;
						int ppos;
						while (pos > 0 && freqs[heap[ppos = (pos - 1) / 2]] > freq) {
							heap[pos] = heap[ppos];
							pos = ppos;
						}
						heap[pos] = n;
						
						maxCode = n;
					}
				}
				
				/* We could encode a single literal with 0 bits but then we
				* don't see the literals.  Therefore we force at least two
				* literals to avoid this case.  We don't care about order in
				* this case, both literals get a 1 bit code.
				*/
				while (heapLen < 2) {
					int node = maxCode < 2 ? ++maxCode : 0;
					heap[heapLen++] = node;
				}
				
				numCodes = Math.Max(maxCode + 1, minNumCodes);
				
				int numLeafs = heapLen;
				int[] childs = new int[4*heapLen - 2];
				int[] values = new int[2*heapLen - 1];
				int numNodes = numLeafs;
				for (int i = 0; i < heapLen; i++) {
					int node = heap[i];
					childs[2*i]   = node;
					childs[2*i+1] = -1;
					values[i] = freqs[node] << 8;
					heap[i] = i;
				}
				
				/* Construct the Huffman tree by repeatedly combining the least two
				* frequent nodes.
				*/
				do {
					int first = heap[0];
					int last  = heap[--heapLen];
					
					/* Propagate the hole to the leafs of the heap */
					int ppos = 0;
					int path = 1;
					
					while (path < heapLen) {
						if (path + 1 < heapLen && values[heap[path]] > values[heap[path+1]]) {
							path++;
						}
							
						heap[ppos] = heap[path];
						ppos = path;
						path = path * 2 + 1;
					}
						
					/* Now propagate the last element down along path.  Normally
					* it shouldn't go too deep.
					*/
					int lastVal = values[last];
					while ((path = ppos) > 0 && values[heap[ppos = (path - 1)/2]] > lastVal) {
						heap[path] = heap[ppos];
					}
					heap[path] = last;
					
					
					int second = heap[0];
					
					/* Create a new node father of first and second */
					last = numNodes++;
					childs[2*last] = first;
					childs[2*last+1] = second;
					int mindepth = Math.Min(values[first] & 0xff, values[second] & 0xff);
					values[last] = lastVal = values[first] + values[second] - mindepth + 1;
					
					/* Again, propagate the hole to the leafs */
					ppos = 0;
					path = 1;
					
					while (path < heapLen) {
						if (path + 1 < heapLen && values[heap[path]] > values[heap[path+1]]) {
							path++;
						}
							
						heap[ppos] = heap[path];
						ppos = path;
						path = ppos * 2 + 1;
					}
						
					/* Now propagate the new element down along path */
					while ((path = ppos) > 0 && values[heap[ppos = (path - 1)/2]] > lastVal) {
						heap[path] = heap[ppos];
					}
					heap[path] = last;
				} while (heapLen > 1);
				
				if (heap[0] != childs.Length / 2 - 1) {
					throw new Exception("Weird!");
				}
				BuildLength(childs);
			}
			
			public int GetEncodedLength()
			{
				int len = 0;
				for (int i = 0; i < freqs.Length; i++) {
					len += freqs[i] * length[i];
				}
				return len;
			}
			
			public void CalcBLFreq(Tree blTree) 
			{
				int max_count;               /* max repeat count */
				int min_count;               /* min repeat count */
				int count;                   /* repeat count of the current code */
				int curlen = -1;             /* length of current code */
				
				int i = 0;
				while (i < numCodes) {
					count = 1;
					int nextlen = length[i];
					if (nextlen == 0) {
						max_count = 138;
						min_count = 3;
					} else {
						max_count = 6;
						min_count = 3;
						if (curlen != nextlen) {
							blTree.freqs[nextlen]++;
							count = 0;
						}
					}
					curlen = nextlen;
					i++;
					
					while (i < numCodes && curlen == length[i]) {
						i++;
						if (++count >= max_count) {
							break;
						}
					}
					
					if (count < min_count) {
						blTree.freqs[curlen] += (short)count;
					} else if (curlen != 0) {
						blTree.freqs[REP_3_6]++;
					} else if (count <= 10) {
						blTree.freqs[REP_3_10]++;
					} else {
						blTree.freqs[REP_11_138]++;
					}
				}
			}
			
			public void WriteTree(Tree blTree)
			{
				int max_count;               /* max repeat count */
				int min_count;               /* min repeat count */
				int count;                   /* repeat count of the current code */
				int curlen = -1;             /* length of current code */
				
				int i = 0;
				while (i < numCodes) {
					count = 1;
					int nextlen = length[i];
					if (nextlen == 0) {
						max_count = 138;
						min_count = 3;
					} else {
						max_count = 6;
						min_count = 3;
						if (curlen != nextlen) {
							blTree.WriteSymbol(nextlen);
							count = 0;
						}
					}
					curlen = nextlen;
					i++;
					
					while (i < numCodes && curlen == length[i]) {
						i++;
						if (++count >= max_count) {
							break;
						}
					}
					
					if (count < min_count) {
						while (count-- > 0) {
							blTree.WriteSymbol(curlen);
						}
					} else if (curlen != 0) {
						blTree.WriteSymbol(REP_3_6);
						dh.pending.WriteBits(count - 3, 2);
					} else if (count <= 10) {
						blTree.WriteSymbol(REP_3_10);
						dh.pending.WriteBits(count - 3, 3);
					} else {
						blTree.WriteSymbol(REP_11_138);
						dh.pending.WriteBits(count - 11, 7);
					}
				}
			}
		}
		
		public DeflaterPending pending;
		private Tree literalTree, distTree, blTree;
		
		private short[] d_buf;
		private byte[]  l_buf;
		private int last_lit;
		private int extra_bits;
		
		private static short[] staticLCodes;
		private static byte[]  staticLLength;
		private static short[] staticDCodes;
		private static byte[]  staticDLength;
		
		/// <summary>
		/// Reverse the bits of a 16 bit value.
		/// </summary>
		public static short BitReverse(int value) 
		{
			return (short) (bit4Reverse[value & 0xF] << 12 | 
			                bit4Reverse[(value >> 4) & 0xF] << 8 | 
			                bit4Reverse[(value >> 8) & 0xF] << 4 |
			                bit4Reverse[value >> 12]);
		}
		
		
		static DeflaterHuffman() 
		{
			/* See RFC 1951 3.2.6 */
			/* Literal codes */
			staticLCodes = new short[LITERAL_NUM];
			staticLLength = new byte[LITERAL_NUM];
			int i = 0;
			while (i < 144) {
				staticLCodes[i] = BitReverse((0x030 + i) << 8);
				staticLLength[i++] = 8;
			}
			while (i < 256) {
				staticLCodes[i] = BitReverse((0x190 - 144 + i) << 7);
				staticLLength[i++] = 9;
			}
			while (i < 280) {
				staticLCodes[i] = BitReverse((0x000 - 256 + i) << 9);
				staticLLength[i++] = 7;
			}
			while (i < LITERAL_NUM) {
				staticLCodes[i] = BitReverse((0x0c0 - 280 + i)  << 8);
				staticLLength[i++] = 8;
			}
			
			/* Distant codes */
			staticDCodes = new short[DIST_NUM];
			staticDLength = new byte[DIST_NUM];
			for (i = 0; i < DIST_NUM; i++) {
				staticDCodes[i] = BitReverse(i << 11);
				staticDLength[i] = 5;
			}
		}
		
		public DeflaterHuffman(DeflaterPending pending)
		{
			this.pending = pending;
			
			literalTree = new Tree(this, LITERAL_NUM, 257, 15);
			distTree    = new Tree(this, DIST_NUM, 1, 15);
			blTree      = new Tree(this, BITLEN_NUM, 4, 7);
			
			d_buf = new short[BUFSIZE];
			l_buf = new byte [BUFSIZE];
		}
		
		public void Reset() 
		{
			last_lit = 0;
			extra_bits = 0;
			literalTree.Reset();
			distTree.Reset();
			blTree.Reset();
		}
		
		int Lcode(int len) 
		{
			if (len == 255) {
				return 285;
			}
			
			int code = 257;
			while (len >= 8) {
				code += 4;
				len >>= 1;
			}
			return code + len;
		}
		
		int Dcode(int distance) 
		{
			int code = 0;
			while (distance >= 4) {
				code += 2;
				distance >>= 1;
			}
			return code + distance;
		}
		
		public void SendAllTrees(int blTreeCodes)
		{
			blTree.BuildCodes();
			literalTree.BuildCodes();
			distTree.BuildCodes();
			pending.WriteBits(literalTree.numCodes - 257, 5);
			pending.WriteBits(distTree.numCodes - 1, 5);
			pending.WriteBits(blTreeCodes - 4, 4);
			for (int rank = 0; rank < blTreeCodes; rank++) {
				pending.WriteBits(blTree.length[BL_ORDER[rank]], 3);
			}
			literalTree.WriteTree(blTree);
			distTree.WriteTree(blTree);
			//			if (DeflaterConstants.DEBUGGING) {
			//				blTree.CheckEmpty();
			//			}
		}
		
		public void CompressBlock()
		{
			for (int i = 0; i < last_lit; i++) {
				int litlen = l_buf[i] & 0xff;
				int dist = d_buf[i];
				if (dist-- != 0) {
					//					if (DeflaterConstants.DEBUGGING) {
					//						Console.Write("["+(dist+1)+","+(litlen+3)+"]: ");
					//					}
					
					int lc = Lcode(litlen);
					literalTree.WriteSymbol(lc);
					
					int bits = (lc - 261) / 4;
					if (bits > 0 && bits <= 5) {
						pending.WriteBits(litlen & ((1 << bits) - 1), bits);
					}
					
					int dc = Dcode(dist);
					distTree.WriteSymbol(dc);
					
					bits = dc / 2 - 1;
					if (bits > 0) {
						pending.WriteBits(dist & ((1 << bits) - 1), bits);
					}
				} else {
					//					if (DeflaterConstants.DEBUGGING) {
					//						if (litlen > 32 && litlen < 127) {
					//							Console.Write("("+(char)litlen+"): ");
					//						} else {
					//							Console.Write("{"+litlen+"}: ");
					//						}
					//					}
					literalTree.WriteSymbol(litlen);
				}
			}
			//			if (DeflaterConstants.DEBUGGING) {
			//				Console.Write("EOF: ");
			//			}
			literalTree.WriteSymbol(EOF_SYMBOL);
			//			if (DeflaterConstants.DEBUGGING) {
			//				literalTree.CheckEmpty();
			//				distTree.CheckEmpty();
			//			}
		}
		
		public void FlushStoredBlock(byte[] stored, int storedOffset, int storedLength, bool lastBlock)
		{
			//			if (DeflaterConstants.DEBUGGING) {
			//				//Console.WriteLine("Flushing stored block "+ storedLength);
			//			}
			pending.WriteBits((DeflaterConstants.STORED_BLOCK << 1) + (lastBlock ? 1 : 0), 3);
			pending.AlignToByte();
			pending.WriteShort(storedLength);
			pending.WriteShort(~storedLength);
			pending.WriteBlock(stored, storedOffset, storedLength);
			Reset();
		}
		
		public void FlushBlock(byte[] stored, int storedOffset, int storedLength, bool lastBlock)
		{
			literalTree.freqs[EOF_SYMBOL]++;
			
			/* Build trees */
			literalTree.BuildTree();
			distTree.BuildTree();
			
			/* Calculate bitlen frequency */
			literalTree.CalcBLFreq(blTree);
			distTree.CalcBLFreq(blTree);
			
			/* Build bitlen tree */
			blTree.BuildTree();
			
			int blTreeCodes = 4;
			for (int i = 18; i > blTreeCodes; i--) {
				if (blTree.length[BL_ORDER[i]] > 0) {
					blTreeCodes = i+1;
				}
			}
			int opt_len = 14 + blTreeCodes * 3 + blTree.GetEncodedLength() + 
				literalTree.GetEncodedLength() + distTree.GetEncodedLength() + 
				extra_bits;
			
			int static_len = extra_bits;
			for (int i = 0; i < LITERAL_NUM; i++) {
				static_len += literalTree.freqs[i] * staticLLength[i];
			}
			for (int i = 0; i < DIST_NUM; i++) {
				static_len += distTree.freqs[i] * staticDLength[i];
			}
			if (opt_len >= static_len) {
				/* Force static trees */
				opt_len = static_len;
			}
			
			if (storedOffset >= 0 && storedLength+4 < opt_len >> 3) {
				/* Store Block */
				//				if (DeflaterConstants.DEBUGGING) {
				//					//Console.WriteLine("Storing, since " + storedLength + " < " + opt_len
				//					                  + " <= " + static_len);
				//				}
				FlushStoredBlock(stored, storedOffset, storedLength, lastBlock);
			} else if (opt_len == static_len) {
				/* Encode with static tree */
				pending.WriteBits((DeflaterConstants.STATIC_TREES << 1) + (lastBlock ? 1 : 0), 3);
				literalTree.SetStaticCodes(staticLCodes, staticLLength);
				distTree.SetStaticCodes(staticDCodes, staticDLength);
				CompressBlock();
				Reset();
			} else {
				/* Encode with dynamic tree */
				pending.WriteBits((DeflaterConstants.DYN_TREES << 1) + (lastBlock ? 1 : 0), 3);
				SendAllTrees(blTreeCodes);
				CompressBlock();
				Reset();
			}
		}
		
		public bool IsFull()
		{
//			return last_lit + 16 >= BUFSIZE; // HACK: This was == 'last_lit == BUFSIZE', but errors occured with DeflateFast
			return last_lit >= BUFSIZE; // -jr- This is the correct form!
		}
		
		public bool TallyLit(int lit)
		{
			//			if (DeflaterConstants.DEBUGGING) {
			//				if (lit > 32 && lit < 127) {
			//					//Console.WriteLine("("+(char)lit+")");
			//				} else {
			//					//Console.WriteLine("{"+lit+"}");
			//				}
			//			}
			d_buf[last_lit] = 0;
			l_buf[last_lit++] = (byte)lit;
			literalTree.freqs[lit]++;
			return IsFull();
		}
		
		public bool TallyDist(int dist, int len)
		{
			//			if (DeflaterConstants.DEBUGGING) {
			//				//Console.WriteLine("["+dist+","+len+"]");
			//			}
			
			d_buf[last_lit]   = (short)dist;
			l_buf[last_lit++] = (byte)(len - 3);
			
			int lc = Lcode(len - 3);
			literalTree.freqs[lc]++;
			if (lc >= 265 && lc < 285) {
				extra_bits += (lc - 261) / 4;
			}
			
			int dc = Dcode(dist - 1);
			distTree.freqs[dc]++;
			if (dc >= 4) {
				extra_bits += dc / 2 - 1;
			}
			return IsFull();
		}
	}
}
