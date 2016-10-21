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

#include <sstream>

#include <gcsa/support.h>

namespace gcsa
{

//------------------------------------------------------------------------------

ConstructionParameters::ConstructionParameters() :
  doubling_steps(DOUBLING_STEPS), size_limit(SIZE_LIMIT * GIGABYTE),
  lcp_branching(LCP_BRANCHING),
  node_mapping(DEFAULT_MAPPING), max_node(DEFAULT_MAX_NODE)
{
}

void
ConstructionParameters::setSteps(size_type steps)
{
  this->doubling_steps = Range::bound(steps, 1, DOUBLING_STEPS);
}

void
ConstructionParameters::setLimit(size_type gigabytes)
{
  this->size_limit = Range::bound(gigabytes, 1, ABSOLUTE_LIMIT) * GIGABYTE;
}

void
ConstructionParameters::setLCPBranching(size_type factor)
{
  this->lcp_branching = std::max((size_type)2, factor);
}

void
ConstructionParameters::setMapping(mapping_type new_mapping)
{
  this->node_mapping = new_mapping;
}

void
ConstructionParameters::setMappingParameter(size_type parameter)
{
  if(this->node_mapping == duplicate_mapping)
  {
    this->max_node = std::max((size_type)1, parameter);
  }
}

void
ConstructionParameters::setMappingParameter(const std::string& parameter)
{
  if(this->node_mapping == duplicate_mapping)
  {
    this->setMappingParameter(std::stoul(parameter));
  }
}

std::string
ConstructionParameters::getMapping() const
{
  std::ostringstream s;
  s << mappingName(this->node_mapping);
  if(this->node_mapping == duplicate_mapping)
  {
    s << "(" << this->max_node << ")";
  }
  return s.str();
}

std::string
ConstructionParameters::mappingName(ConstructionParameters::mapping_type mapping)
{
  switch(mapping)
  {
  case identity_mapping:
    return "identity";
  case duplicate_mapping:
    return "duplicate";
  default:
    return "unknown";
  }
}

ConstructionParameters::mapping_type
ConstructionParameters::mappingType(const std::string& mapping)
{
  if(mapping == "identity") { return identity_mapping; }
  if(mapping == "duplicate") { return duplicate_mapping; }

  std::cerr << "ConstructionParameters::mappingType(): Unknown mapping type: " << mapping << std::endl;
  return DEFAULT_MAPPING;
}

//------------------------------------------------------------------------------

/*
  The default alphabet interprets \0 and $ as endmarkers, ACGT and acgt as ACGT,
  # as a the label of the source node, and the and the remaining characters as N.
*/

const sdsl::int_vector<8> Alphabet::DEFAULT_CHAR2COMP =
{
  0, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 6,  0, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 1, 5, 2,  5, 5, 5, 3,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  4, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 1, 5, 2,  5, 5, 5, 3,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  4, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,

  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,
  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5,  5, 5, 5, 5
};

const sdsl::int_vector<8> Alphabet::DEFAULT_COMP2CHAR = { '$', 'A', 'C', 'G', 'T', 'N', '#' };

//------------------------------------------------------------------------------

Alphabet::Alphabet() :
  char2comp(DEFAULT_CHAR2COMP), comp2char(DEFAULT_COMP2CHAR),
  C(DEFAULT_COMP2CHAR.size() + 1, 0),
  sigma(DEFAULT_COMP2CHAR.size()), fast_chars(FAST_CHARS)
{
}

Alphabet::Alphabet(const Alphabet& a)
{
  this->copy(a);
}

Alphabet::Alphabet(Alphabet&& a)
{
  *this = std::move(a);
}

Alphabet::Alphabet(const sdsl::int_vector<64>& counts,
  const sdsl::int_vector<8>& _char2comp, const sdsl::int_vector<8>& _comp2char) :
  char2comp(_char2comp), comp2char(_comp2char),
  C(_comp2char.size() + 1, 0),
  sigma(_comp2char.size()), fast_chars(FAST_CHARS)
{
  for(size_type i = 0; i < counts.size(); i++) { this->C[i + 1] = this->C[i] + counts[i]; }
}

Alphabet::~Alphabet()
{
}

void
Alphabet::copy(const Alphabet& a)
{
  this->char2comp = a.char2comp;
  this->comp2char = a.comp2char;
  this->C = a.C;
  this->sigma = a.sigma;
  this->fast_chars = a.fast_chars;
}

void
Alphabet::swap(Alphabet& a)
{
  if(this != &a)
  {
    this->char2comp.swap(a.char2comp);
    this->comp2char.swap(a.comp2char);
    this->C.swap(a.C);
    std::swap(this->sigma, a.sigma);
    std::swap(this->fast_chars, a.fast_chars);
  }
}

Alphabet&
Alphabet::operator=(const Alphabet& a)
{
  if(this != &a) { this->copy(a); }
  return *this;
}

Alphabet&
Alphabet::operator=(Alphabet&& a)
{
  if(this != &a)
  {
    this->char2comp = std::move(a.char2comp);
    this->comp2char = std::move(a.comp2char);
    this->C = std::move(a.C);
    this->sigma = a.sigma;
    this->fast_chars = a.fast_chars;
  }
  return *this;
}

Alphabet::size_type
Alphabet::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;
  written_bytes += this->char2comp.serialize(out, child, "char2comp");
  written_bytes += this->comp2char.serialize(out, child, "comp2char");
  written_bytes += this->C.serialize(out, child, "C");
  written_bytes += sdsl::write_member(this->sigma, out, child, "sigma");
  written_bytes += sdsl::write_member(this->fast_chars, out, child, "fast_chars");
  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
Alphabet::load(std::istream& in)
{
  this->char2comp.load(in);
  this->comp2char.load(in);
  this->C.load(in);
  sdsl::read_member(this->sigma, in);
  sdsl::read_member(this->fast_chars, in);
}

//------------------------------------------------------------------------------

SadaCount::SadaCount()
{
}

SadaCount::SadaCount(const SadaCount& source)
{
  this->copy(source);
}

SadaCount::SadaCount(SadaCount&& source)
{
  *this = std::move(source);
}

SadaCount::~SadaCount()
{
}

void
SadaCount::swap(SadaCount& another)
{
  if(this != &another)
  {
    this->data.swap(another.data);
    sdsl::util::swap_support(this->select, another.select, &(this->data), &(another.data));
  }
}

SadaCount&
SadaCount::operator=(const SadaCount& source)
{
  if(this != &source) { this->copy(source); }
  return *this;
}

SadaCount&
SadaCount::operator=(SadaCount&& source)
{
  if(this != &source)
  {
    this->data = std::move(source.data);
    this->select = std::move(source.select);
    this->setVectors();
  }
  return *this;
}

SadaCount::size_type
SadaCount::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;

  written_bytes += this->data.serialize(out, child, "data");
  written_bytes += this->select.serialize(out, child, "select");

  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
SadaCount::load(std::istream& in)
{
  this->data.load(in);
  this->select.load(in, &(this->data));
}

void
SadaCount::copy(const SadaCount& source)
{
  this->data = source.data;
  this->select = source.select;
  this->setVectors();
}

void
SadaCount::setVectors()
{
  this->select.set_vector(&(this->data));
}

//------------------------------------------------------------------------------

SadaSparse::SadaSparse()
{
}

SadaSparse::SadaSparse(const SadaSparse& source)
{
  this->copy(source);
}

SadaSparse::SadaSparse(SadaSparse&& source)
{
  *this = std::move(source);
}

SadaSparse::~SadaSparse()
{
}

void
SadaSparse::swap(SadaSparse& another)
{
  if(this != &another)
  {
    this->filter.swap(another.filter);
    sdsl::util::swap_support(this->filter_rank, another.filter_rank,
      &(this->filter), &(another.filter));

    this->values.swap(another.values);
    sdsl::util::swap_support(this->value_select, another.value_select,
      &(this->values), &(another.values));
  }
}

SadaSparse&
SadaSparse::operator=(const SadaSparse& source)
{
  if(this != &source) { this->copy(source); }
  return *this;
}

SadaSparse&
SadaSparse::operator=(SadaSparse&& source)
{
  if(this != &source)
  {
    this->filter = std::move(source.filter);
    this->filter_rank = std::move(source.filter_rank);

    this->values = std::move(source.values);
    this->value_select = std::move(source.value_select);

    this->setVectors();
  }
  return *this;
}

SadaSparse::size_type
SadaSparse::serialize(std::ostream& out, sdsl::structure_tree_node* v, std::string name) const
{
  sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
  size_type written_bytes = 0;

  written_bytes += this->filter.serialize(out, child, "filter");
  written_bytes += this->filter_rank.serialize(out, child, "filter_rank");

  written_bytes += this->values.serialize(out, child, "values");
  written_bytes += this->value_select.serialize(out, child, "value_select");

  sdsl::structure_tree::add_size(child, written_bytes);
  return written_bytes;
}

void
SadaSparse::load(std::istream& in)
{
  this->filter.load(in);
  this->filter_rank.load(in, &(this->filter));

  this->values.load(in);
  this->value_select.load(in, &(this->values));
}

void
SadaSparse::copy(const SadaSparse& source)
{
  this->filter = source.filter;
  this->filter_rank = source.filter_rank;

  this->values = source.values;
  this->value_select = source.value_select;

  this->setVectors();
}

void
SadaSparse::setVectors()
{
  this->filter_rank.set_vector(&(this->filter));
  this->value_select.set_vector(&(this->values));
}

//------------------------------------------------------------------------------

std::string
Key::decode(key_type key, size_type kmer_length, const Alphabet& alpha)
{
  key = label(key);
  size_type max_length = Key::MAX_LENGTH;  // avoid direct use of static const
  kmer_length = std::min(kmer_length, max_length);

  std::string res(kmer_length, '\0');
  for(size_type i = 1; i <= kmer_length; i++)
  {
    res[kmer_length - i] = alpha.comp2char[key & CHAR_MASK];
    key >>= CHAR_WIDTH;
  }

  return res;
}

void
Key::lastChars(const std::vector<key_type>& keys, sdsl::int_vector<0>& last_char)
{
  sdsl::util::clear(last_char);
  last_char = sdsl::int_vector<0>(keys.size(), 0, CHAR_WIDTH);
  for(size_type i = 0; i < keys.size(); i++) { last_char[i] = Key::last(keys[i]); }
}

//------------------------------------------------------------------------------

node_type
Node::encode(const std::string& token)
{
  size_t separator = 0;
  size_type node = std::stoul(token, &separator);
  if(separator + 1 >= token.length())
  {
    std::cerr << "Node::encode(): Invalid position token " << token << std::endl;
    return 0;
  }

  bool reverse_complement = false;
  if(token[separator + 1] == '-')
  {
    reverse_complement = true;
    separator++;
  }

  std::string temp = token.substr(separator + 1);
  size_type offset = std::stoul(temp);
  if(offset > OFFSET_MASK)
  {
    std::cerr << "Node::encode(): Offset " << offset << " too large" << std::endl;
    return 0;
  }

  return encode(node, offset, reverse_complement);
}

std::string
Node::decode(node_type node)
{
  std::ostringstream ss;
  ss << id(node) << ':';
  if(rc(node)) { ss << '-'; }
  ss << offset(node);
  return ss.str();
}

//------------------------------------------------------------------------------

KMer::KMer()
{
}

KMer::KMer(const std::vector<std::string>& tokens, const Alphabet& alpha, size_type successor)
{
  byte_type predecessors = chars(tokens[2], alpha);
  byte_type successors = chars(tokens[3], alpha);
  this->key = Key::encode(alpha, tokens[0], predecessors, successors);
  this->from = Node::encode(tokens[1]);
  this->to = Node::encode(tokens[successor]);
}

byte_type
KMer::chars(const std::string& token, const Alphabet& alpha)
{
  byte_type val = 0;
  for(size_type i = 0; i < token.length(); i += 2) { val |= 1 << alpha.char2comp[token[i]]; }
  return val;
}

std::ostream&
operator<< (std::ostream& out, const KMer& kmer)
{
  out << "(key " << Key::label(kmer.key)
      << ", in " << (size_type)(Key::predecessors(kmer.key))
      << ", out " << (size_type)(Key::successors(kmer.key))
      << ", from " << Node::decode(kmer.from)
      << ", to " << Node::decode(kmer.to) << ")";
  return out;
}

//------------------------------------------------------------------------------

} // namespace gcsa
