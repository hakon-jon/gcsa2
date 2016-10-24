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

#ifndef _GCSA_FILES_H
#define _GCSA_FILES_H

#include <gcsa/support.h>

namespace gcsa
{

/*
  files.h: Public interface for file formats.
*/

//------------------------------------------------------------------------------

struct GraphFileHeader
{
  size_type flags;
  size_type kmer_count;
  size_type kmer_length;

  GraphFileHeader();
  GraphFileHeader(size_type kmers, size_type length);
  explicit GraphFileHeader(std::istream& in);
  ~GraphFileHeader();

  size_type serialize(std::ostream& out);
};

/*
  These functions read the input until eof. They do not close the input stream. The
  return value is kmer length.
*/
size_type readBinary(std::istream& in, std::vector<KMer>& kmers, bool append = false);
size_type readText(std::istream& in, std::vector<KMer>& kmers, const Alphabet& alpha, bool append = false);

// FIXME Later: writeText()
void writeBinary(std::ostream& out, std::vector<KMer>& kmers, size_type kmer_length);
void writeKMers(const std::string& base_name, std::vector<KMer>& kmers, size_type kmer_length);

//------------------------------------------------------------------------------

/*
  An input graph is just a set of input files.
*/

struct InputGraph
{
  std::vector<std::string> filenames;
  std::string              lcp_name; // Used to pass the LCP array from GCSA construction.
  std::vector<size_type>   sizes;

  Alphabet                 alpha;

  bool binary;
  size_type kmer_count, kmer_length;


  const static size_type UNKNOWN = ~(size_type)0;
  const static std::string BINARY_EXTENSION;  // .graph
  const static std::string TEXT_EXTENSION;    // .gcsa2

  InputGraph(const std::vector<std::string>& files, bool binary_format, const Alphabet& alphabet = Alphabet());
  InputGraph(size_type file_count, char** base_names, bool binary_format, const Alphabet& alphabet = Alphabet());
  ~InputGraph();

  void open(std::ifstream& input, size_type file) const;
  void setK(size_type new_k, size_type file);
  void checkK(size_type new_k, size_type file) const;

  inline size_type size() const { return this->kmer_count; }
  inline size_type k() const { return this->kmer_length; }
  inline size_type files() const { return this->filenames.size(); }

  /*
    Setting append = true has unpredictable side effects if done outside the member
    functions of InputGraph.
  */
  void read(std::vector<KMer>& kmers) const;
  void read(std::vector<KMer>& kmers, size_type file, bool append = false) const;

  void readKeys(std::vector<key_type>& keys) const;
  void readFrom(std::vector<node_type>& from_nodes, const ConstructionParameters& parameters = ConstructionParameters()) const;

  InputGraph(const InputGraph&) = delete;
  InputGraph& operator= (const InputGraph&) = delete;

private:
  void build();
};

//------------------------------------------------------------------------------

/*
  GCSA file header.

  Version 3 (GCSA v0.8):
  - Changed to a faster CSA-style encoding.

  Version 2 (GCSA v0.6):
  - Added OccurrenceCounter to the end of the body.

  Version 1 (GCSA v0.5)
  - The first use of the header.
  - GCSA body is identical to version 0.
*/

struct GCSAHeader
{
  uint32_t tag;
  uint32_t version;
  uint64_t path_nodes;
  uint64_t edges;
  uint64_t order;
  uint64_t flags;

  const static uint32_t TAG = 0x6C5A6C5A;
  const static uint32_t VERSION = 3;
  const static uint32_t MIN_VERSION = 1;

  const static uint64_t COMPRESSED = 0x1; // Not in use.

  GCSAHeader();

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);
  bool check(uint32_t expected_version = VERSION) const;
  bool checkNew() const;

  void swap(GCSAHeader& another);
};

std::ostream& operator<<(std::ostream& stream, const GCSAHeader& header);

//------------------------------------------------------------------------------

/*
  LCP file header.

  Version 1 (GCSA v0.8)
  - The first use of the header.
  - LCP body is identical to version 0.
*/

struct LCPHeader
{
  uint32_t tag;
  uint32_t version;
  uint64_t size;
  uint64_t branching;
  uint64_t flags;

  const static uint32_t TAG = 0x6C5A7C94;
  const static uint32_t VERSION = 1;
  const static uint32_t MIN_VERSION = 1;

  LCPHeader();

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);
  bool check(uint32_t expected_version = VERSION) const;
  bool checkNew() const;

  void swap(LCPHeader& another);
};

std::ostream& operator<<(std::ostream& stream, const LCPHeader& header);

//------------------------------------------------------------------------------

} // namespace gcsa

#endif // _GCSA_UTILS_H
