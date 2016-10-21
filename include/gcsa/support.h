/*
  Copyright (c) 2015, 2016 Genome Research Ltd.

  Author: Jouni Siren <jouni.siren@iki.fi>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef _GCSA_SUPPORT_H
#define _GCSA_SUPPORT_H

#include <gcsa/utils.h>

namespace gcsa
{

/*
  support.h: Support structures included in the public interface.
*/

//------------------------------------------------------------------------------

struct ConstructionParameters
{
  enum mapping_type { identity_mapping, duplicate_mapping };

  const static size_type    DOUBLING_STEPS   = 3;
  const static size_type    SIZE_LIMIT       = 500;   // Gigabytes.
  const static size_type    ABSOLUTE_LIMIT   = 16384; // Gigabytes.
  const static size_type    LCP_BRANCHING    = 64;
  const static mapping_type DEFAULT_MAPPING  = identity_mapping;
  const static size_type    DEFAULT_MAX_NODE = ((size_type)1 << 53) - 2;  // Modulo becomes ~0.

  ConstructionParameters();
  void setSteps(size_type steps);
  void setLimit(size_type gigabytes);
  void setLCPBranching(size_type factor);

  void setMapping(mapping_type new_mapping);
  void setMappingParameter(size_type parameter);
  void setMappingParameter(const std::string& parameter);
  std::string getMapping() const;

  static std::string mappingName(mapping_type mapping);
  static mapping_type mappingType(const std::string& mapping);

  size_type    doubling_steps;
  size_type    size_limit;
  size_type    lcp_branching;
  mapping_type node_mapping;
  size_type    max_node;
};

//------------------------------------------------------------------------------

template<class ByteVector>
void characterCounts(const ByteVector& sequence, const sdsl::int_vector<8>& char2comp, sdsl::int_vector<64>& counts);

/*
  This replaces the SDSL byte_alphabet. The main improvements are:
    - The alphabet can be built from an existing sequence.
    - The comp order does not need to be the same as character order, as long as \0 is the first character.
*/

class Alphabet
{
public:
  typedef gcsa::size_type size_type;
  const static size_type MAX_SIGMA = 256;

  // Comp values for source/sink markers in the default alphabet.
  const static size_type SOURCE_COMP = 6;
  const static size_type SINK_COMP = 0;

  // By default, comp values 1 to 4 should be encoded using the fast encoding.
  const static size_type FAST_CHARS = 4;

  const static sdsl::int_vector<8> DEFAULT_CHAR2COMP;
  const static sdsl::int_vector<8> DEFAULT_COMP2CHAR;

  Alphabet();
  Alphabet(const Alphabet& a);
  Alphabet(Alphabet&& a);
  ~Alphabet();

  /*
    ByteVector only has to support operator[] and size(). If there is a clearly faster way for
    sequential access, function characterCounts() should be specialized.
  */
  template<class ByteVector>
  explicit Alphabet(const ByteVector& sequence,
    const sdsl::int_vector<8>& _char2comp = DEFAULT_CHAR2COMP,
    const sdsl::int_vector<8>& _comp2char = DEFAULT_COMP2CHAR) :
    char2comp(_char2comp), comp2char(_comp2char),
    C(sdsl::int_vector<64>(_comp2char.size() + 1, 0)),
    sigma(_comp2char.size()), fast_chars(FAST_CHARS)
  {
    if(sequence.size() == 0) { return; }

    characterCounts(sequence, this->char2comp, this->C);
    for(size_type i = 0, sum = 0; i < this->C.size(); i++)
    {
      size_type temp = this->C[i]; this->C[i] = sum; sum += temp;
    }
  }

  /*
    The counts array holds character counts for all comp values.
  */
  explicit Alphabet(const sdsl::int_vector<64>& counts,
    const sdsl::int_vector<8>& _char2comp = DEFAULT_CHAR2COMP,
    const sdsl::int_vector<8>& _comp2char = DEFAULT_COMP2CHAR);

  void swap(Alphabet& a);
  Alphabet& operator=(const Alphabet& a);
  Alphabet& operator=(Alphabet&& a);

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);

  sdsl::int_vector<8>  char2comp, comp2char;
  sdsl::int_vector<64> C;
  size_type            sigma, fast_chars;

private:
  void copy(const Alphabet& a);
};  // class Alphabet

template<class ByteVector>
void
characterCounts(const ByteVector& sequence, const sdsl::int_vector<8>& char2comp, sdsl::int_vector<64>& counts)
{
  for(size_type c = 0; c < counts.size(); c++) { counts[c] = 0; }
  for(size_type i = 0; i < sequence.size(); i++) { counts[char2comp[sequence[i]]]++; }
}

//------------------------------------------------------------------------------

/*
  The core part of Sadakane's document counting structure. Essentially just unary encoding
  of array A of non-negative integers. Query count(sp, ep) returns A[sp] + ... + A[ep].
*/

class SadaCount
{
public:
  typedef gcsa::size_type  size_type;
  typedef sdsl::bit_vector bit_vector;

  SadaCount();
  SadaCount(const SadaCount& source);
  SadaCount(SadaCount&& source);
  ~SadaCount();

  template<class Container> explicit SadaCount(const Container& source);

  void swap(SadaCount& another);
  SadaCount& operator=(const SadaCount& source);
  SadaCount& operator=(SadaCount&& source);

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);

  // Value k is encoded as 0^k 1.
  bit_vector                data;
  bit_vector::select_1_type select;

  inline size_type count(size_type sp, size_type ep) const
  {
    return (this->select(ep + 1) - ep) - (sp > 0 ? this->select(sp) + 1 - sp : 0);
  }

private:
  void copy(const SadaCount& source);
  void setVectors();
};

template<class Container>
SadaCount::SadaCount(const Container& source)
{
  size_type total = 0;
  for(size_type i = 0; i < source.size(); i++) { total += source[i]; }

  this->data = bit_vector(source.size() + total, 0);
  for(size_type i = 0, tail = 0; i < source.size(); i++)
  {
    tail += source[i] + 1;
    this->data[tail - 1] = 1;
  }

  sdsl::util::init_support(this->select, &(this->data));
}

//------------------------------------------------------------------------------

/*
  This version of Sadakane's document counting structure uses a sparse filter. In
  the terminology of (Gagie et al: Document Counting in Compressed Space. DCC 2015)
  this means Sada-S.

  The "best" approach is to use SadaSparse for extra_pointers and SadaCount for
  redundant_pointers. If we use other encodings for redundant_pointers, we get
  roughly the following time/space trade-offs with whole-genome graphs:

                    Time vs. Sada       Space vs. Sada
      Sada-S        1.25x               1x
      Sada-S-S      1.5x                0.75-0.8x
      Sada-RS-S     1.5-2.5x            0.8-0.85x
*/

class SadaSparse
{
public:
  typedef gcsa::size_type   size_type;
  typedef sdsl::sd_vector<> sd_vector;

  SadaSparse();
  SadaSparse(const SadaSparse& source);
  SadaSparse(SadaSparse&& source);
  ~SadaSparse();

  template<class Container> explicit SadaSparse(const Container& source);

  void swap(SadaSparse& another);
  SadaSparse& operator=(const SadaSparse& source);
  SadaSparse& operator=(SadaSparse&& source);

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);

  // Positions with non-zero values are marked with an 1-bit.
  sd_vector                filter;
  sd_vector::rank_1_type   filter_rank;

  // Non-zero values encoded in unary: k becomes 0^{k-1} 1.
  sd_vector                values;
  sd_vector::select_1_type value_select;

  // The number of nonzero values.
  inline size_type items() const { return this->filter_rank(this->filter.size()); }

  inline size_type count(size_type sp, size_type ep) const
  {
    sp = this->filter_rank(sp);     // Closed lower bound for ranks of filtered values.
    ep = this->filter_rank(ep + 1); // Open upper bound for ranks of filtered values.
    if(ep <= sp) { return 0; }
    return (this->value_select(ep) + 1) - (sp > 0 ? this->value_select(sp) + 1 : 0);
  }

private:
  void copy(const SadaSparse& source);
  void setVectors();
};

template<class Container>
SadaSparse::SadaSparse(const Container& source)
{
  // Sparse filter.
  size_type total = 0, filtered_values = 0;
  sdsl::bit_vector buffer(source.size(), 0);
  for(size_type i = 0; i < source.size(); i++)
  {
    if(source[i] > 0) { buffer[i] = 1; total += source[i]; filtered_values++; }
  }
  this->filter = sd_vector(buffer); sdsl::util::clear(buffer);

  // Filtered values.
  sdsl::sd_vector_builder builder(total, filtered_values);
  for(size_type i = 0, tail = 0; i < source.size(); i++)
  {
    if(source[i] > 0) { tail += source[i]; builder.set(tail - 1); }
  }
  this->values = sd_vector(builder);

  sdsl::util::init_support(this->filter_rank, &(this->filter));
  sdsl::util::init_support(this->value_select, &(this->values));
}

//------------------------------------------------------------------------------

/*
  This interface is intended for indexing kmers of length 16 or less on an alphabet of size
  8 or less. The kmer is encoded as an 64-bit integer (most significant bit first):
    - 16x3 bits for the label, with high-order characters 0s when necessary
    - 8 bits for marking which predecessors are present
    - 8 bits for marking which successors are present
*/

typedef std::uint64_t key_type;

struct Key
{
  const static size_type CHAR_WIDTH = 3;
  const static key_type  CHAR_MASK = 0x7;
  const static size_type MAX_LENGTH = 16;
  const static key_type  PRED_SUCC_MASK = 0xFFFF;

  inline static key_type encode(const Alphabet& alpha, const std::string& kmer,
    byte_type pred, byte_type succ)
  {
    key_type value = 0;
    for(size_type i = 0; i < kmer.length(); i++)
    {
      value = (value << CHAR_WIDTH) | alpha.char2comp[kmer[i]];
    }
    value = (value << 8) | pred;
    value = (value << 8) | succ;
    return value;
  }

  static std::string decode(key_type key, size_type kmer_length, const Alphabet& alpha);

  inline static size_type label(key_type key) { return (key >> 16); }
  inline static byte_type predecessors(key_type key) { return (key >> 8) & 0xFF; }
  inline static byte_type successors(key_type key) { return key & 0xFF; }
  inline static comp_type last(key_type key) { return (key >> 16) & CHAR_MASK; }

  inline static key_type merge(key_type key1, key_type key2) { return (key1 | (key2 & PRED_SUCC_MASK)); }
  inline static key_type replace(key_type key, size_type kmer_val)
  {
    return (kmer_val << 16) | (key & PRED_SUCC_MASK);
  }

  inline static size_type lcp(key_type a, key_type b, size_type kmer_length)
  {
    size_type res = 0;
    key_type mask = CHAR_MASK << (CHAR_WIDTH * kmer_length);
    a = label(a); b = label(b);

    while(mask > 0)
    {
      mask >>= CHAR_WIDTH;
      if((a & mask) != (b & mask)) { break; }
      res++;
    }

    return res;
  }

  static void lastChars(const std::vector<key_type>& keys, sdsl::int_vector<0>& last_char);
};

//------------------------------------------------------------------------------

/*
  node_type is the identifier of a node in the input graph. It corresponds to a
  position in the original graph.

  The identifier contains 53 bits for node id, 1 bit for orientation (forward or
  reverse complement, and 10 bits for node offset. The string representation of
  a node_type is id:offset for forward positions and id:-offset for reverse
  complement positions. If the forward offsets are 0 to k, the corresponding
  reverse complement offsets are -k to -0 (in the same order).
*/

typedef std::uint64_t node_type;

struct Node
{
  const static size_type ID_OFFSET        = 11;
  const static size_type ORIENTATION_MASK = 0x400;
  const static size_type OFFSET_MASK      = 0x3FF;

  inline static node_type encode(size_type node_id, size_type node_offset)
  {
    return (node_id << ID_OFFSET) | node_offset;
  }

  inline static node_type encode(size_type node_id, size_type node_offset, bool reverse_complement)
  {
    return encode(node_id, node_offset) | (reverse_complement ? ORIENTATION_MASK : 0);
  }

  static node_type encode(const std::string& token);
  static std::string decode(node_type node);

  inline static size_type id(node_type node) { return node >> ID_OFFSET; }
  inline static bool rc(node_type node) { return node & ORIENTATION_MASK; }
  inline static size_type offset(node_type node) { return node & OFFSET_MASK; }
};

//------------------------------------------------------------------------------

/*
  Node mappings map the node identifiers used in prefix-doubling to the values returned
  by a locate() query. The following mappings have been implemented:

    NodeIdentityMapping   Use the identifiers as values (default)
    NodeDuplicateMapping  Maps the duplicated nodes in haplotype paths to the originals
*/

struct NodeIdentityMapping
{
  inline void add(std::vector<node_type>& nodes, node_type identifier) const
  {
    nodes.push_back(identifier);
  }
};

struct NodeDuplicateMapping
{
  node_type modulo;

  explicit NodeDuplicateMapping(size_type max_node) : modulo((max_node + 1) << Node::ID_OFFSET) {}

  inline void add(std::vector<node_type>& nodes, node_type identifier) const
  {
    nodes.push_back(identifier % this->modulo);
  }
};

//------------------------------------------------------------------------------

struct KMer
{
  key_type  key;
  node_type from, to;

  KMer();
  KMer(const std::vector<std::string>& tokens, const Alphabet& alpha, size_type successor);

  KMer(key_type _key, node_type _from, node_type _to) :
    key(_key), from(_from), to(_to)
  {
  }

  inline bool
  operator< (const KMer& another) const
  {
    return (Key::label(this->key) < Key::label(another.key));
  }

  inline bool sorted() const { return (this->to == ~(node_type)0); }
  inline void makeSorted() { this->to = ~(node_type)0; }

  static byte_type chars(const std::string& token, const Alphabet& alpha);
};

std::ostream& operator<< (std::ostream& out, const KMer& kmer);

inline bool
operator< (key_type key, const KMer& kmer)
{
  return (Key::label(key) < Key::label(kmer.key));
}

//------------------------------------------------------------------------------

} // namespace gcsa

#endif // _GCSA_SUPPORT_H
