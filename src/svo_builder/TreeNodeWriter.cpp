#include "TreeNodeWriter.h"

TreeNodeWriter::TreeNodeWriter(): file_pointer(nullptr), position_in_output_file(0), hasBeenClosed(false)
{
}

TreeNodeWriter::TreeNodeWriter(std::string base_filename): position_in_output_file(0), hasBeenClosed(false)
{
	string nodes_name = base_filename + string(".tree4dnodes");
	file_pointer = fopen(nodes_name.c_str(), "wb");
	if(file_pointer == nullptr)
	{
		std::cout << ".tree4dnodes-file is not or incorrectly opened.";
	}
}

TreeNodeWriter::~TreeNodeWriter()
{
	closeFile();
}

void TreeNodeWriter::closeFile()
{ 
	if (!hasBeenClosed)
	{
		fclose(file_pointer);
	}
}

size_t TreeNodeWriter::writeNode4D_(const Node4D &node)
{
	/* https://github.com/Forceflow/ooc_svo_builder
	An .tree4dnodes file is a binary file
	which describes the big flat array of tree4D nodes.
	In the nodes, there are only child pointers,
	which are constructed from a 64-bit base address
	combined with a child offset,
	since all nonempty children of a certain node are guaranteed by the algorithm
	to be stored next to eachother.
	The .tree4dnodes file contains an amount of n_nodes nodes.

	SO:
	For each node,
	write 3 elements to the file
	1) the base address of the children of this node
	=> size_t, 64 bits
	2) the child ofsets for each of the 8 children.
	=> 16 children * 8 bits offset = 128 bits
	3) the data address = Index of data payload in data array described in the .octreedata file (see further)
	=> size_t, 64 bits
	==> 4 * 64 bits

	Node4D has as member variables:
	size_t data; (64 bits)
	size_t children_base; (64 bits)
	char children_offset[16]; (128 bits = 2 * 64 bits)
	*/

	if (file_pointer == nullptr)
	{
		std::cout << ".tree4dnodes-file is not or incorrectly opened.";
	}

	fwrite(&node.data, sizeof(size_t), 4, file_pointer);
	/*
	size_t fwrite ( const void * ptr, size_t size, size_t count, FILE * stream );
	Write block of data to stream.

	Writes an array of count elements,
	each one with a size of size bytes,
	from the block of memory pointed by ptr to the current position in the stream.*/

	position_in_output_file++;
	return position_in_output_file - 1;
}

