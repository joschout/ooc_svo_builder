//#include "partitioner.h"
//
//using namespace std;
//using namespace trimesh;
//
//
//// Fiddle with buffer sizes here: these are defined as number of triangles
//#define input_buffersize 8192
//#define output_buffersize 8192
//
//
//// Estimate the optimal amount of partitions we need, given the requested gridsize and the overall memory limit.
//size_t estimate_partitions(const size_t gridsize, const size_t memory_limit, const size_t nb_dimensions) {
//	cout << "Estimating best partition count ..." << endl;
//
//	// calculate the amount of memory needed (in MB) to do the partitioning completely in memory
//	/* amount of memory needed
//	= amount of voxels in the high-resolution 3D grid
//	* the size of a character
//	* 1/1024 (1kB/1024bytes)
//	* 1/1024 (1MB/kB)
//	*/
//
//	size_t gridsize_powered = pow(gridsize, nb_dimensions);
//	uint64_t required = (gridsize_powered*sizeof(char)) / 1024 / 1024;
//	cout << "  to do this in-core I would need " << required << " Mb of system memory" << endl;
//	if (required <= memory_limit) {
//		cout << "  memory limit of " << memory_limit << " Mb allows that" << endl;
//		return 1;
//	}
//	size_t numpartitions = 1;
//	size_t partition_size = required;
//	auto partitioning_amount = pow(2, nb_dimensions);
//	while (partition_size > memory_limit) {
//		partition_size = partition_size / partitioning_amount;
//		numpartitions = numpartitions * partitioning_amount;
//	}
//	cout << "  going to do it in " << numpartitions << " partitions of " << partition_size << " Mb each." << endl;
//	return numpartitions;
//}
//
//// Remove the temporary .trip files we made
//void removeTripFiles(const TripInfo &trip_info) {
//	// remove header file
//	string filename = trip_info.base_filename + string(".trip");
//	remove(filename.c_str());
//	// remove tripdata files
//	for (size_t i = 0; i < trip_info.n_partitions; i++) {
//		filename = trip_info.base_filename + string("_") + val_to_string(i) + string(".tripdata");
//		remove(filename.c_str());
//	}
//}
//
//// Create as many Buffers as there are partitions for a total gridsize,
//// store them in the given vector,
//// use tri_info for filename information
//void createBuffers(const TriInfo& tri_info, const size_t n_partitions, const size_t gridsize, vector<Buffer*> &buffers) {
//	buffers.reserve(n_partitions);
//
//	//the unit length in the grid = the length of one side of the grid
//	// divided by the size of the grid (i.e. the number of voxels next to each other)
//	float unitlength = (tri_info.mesh_bbox.max[0] - tri_info.mesh_bbox.min[0]) / (float)gridsize;
//	uint64_t morton_part = pow(gridsize, 3) / n_partitions;
//
//	AABox<uivec3> bbox_grid;
//	AABox<vec3> bbox_world;
//	std::string filename;
//
//	for (size_t i = 0; i < n_partitions; i++) {
//		// compute world bounding box
//		morton3D_64_decode(morton_part*i, bbox_grid.min[2], bbox_grid.min[1], bbox_grid.min[0]);
//		morton3D_64_decode((morton_part*(i + 1)) - 1, bbox_grid.max[2], bbox_grid.max[1], bbox_grid.max[0]); // -1, because z-curve skips to first block of next partition
//		bbox_world.min[0] = bbox_grid.min[0] * unitlength;
//		bbox_world.min[1] = bbox_grid.min[1] * unitlength;
//		bbox_world.min[2] = bbox_grid.min[2] * unitlength;
//		bbox_world.max[0] = (bbox_grid.max[0] + 1)*unitlength; // + 1, to include full last block
//		bbox_world.max[1] = (bbox_grid.max[1] + 1)*unitlength;
//		bbox_world.max[2] = (bbox_grid.max[2] + 1)*unitlength;
//
//		// output partition info
//		if (verbose) {
//			cout << "Partitioning partition #" << i + 1 << " / " << n_partitions << " id: " << i << " ..." << endl;
//			cout << "  morton from " << morton_part*i << " to " << morton_part*(i + 1) << endl;
//			cout << "  grid coordinates from (" << bbox_grid.min[0] << "," << bbox_grid.min[1] << "," << bbox_grid.min[2] << ") to ("
//				<< bbox_grid.max[0] << "," << bbox_grid.max[1] << "," << bbox_grid.max[2] << ")" << endl;
//			cout << "  worldspace coordinates from (" << bbox_world.min[0] << "," << bbox_world.min[1] << "," << bbox_world.min[2] << ") to ("
//				<< bbox_world.max[0] << "," << bbox_world.max[1] << "," << bbox_world.max[2] << ")" << endl;
//		}
//
//		// create buffer for partition
//		filename = tri_info.base_filename + val_to_string(gridsize) + string("_") + val_to_string(n_partitions) + string("_") + val_to_string(i) + string(".tripdata");
//		buffers[i] = new Buffer(filename, bbox_world, output_buffersize);
//	}
//}
//
//// Handle the special case of just needing one partition
//TripInfo partition_one(const TriInfo& tri_info, const size_t gridsize) {
//	// Just copy files
//	string src = tri_info.base_filename + string(".tridata");
//	string dst 
//		= tri_info.base_filename + val_to_string(gridsize) 
//		+ string("_") + val_to_string(1) 
//		+ string("_") + val_to_string(0) + string(".tripdata");
//	copy_file(src, dst);
//
//	// Write header
//	TripInfo trip_info = TripInfo(tri_info);
//	trip_info.part_tricounts.resize(1);
//	trip_info.part_tricounts[0] = tri_info.n_triangles;
//	trip_info.base_filename = tri_info.base_filename + val_to_string(gridsize) + string("_") + val_to_string(1);
//	std::string header = trip_info.base_filename + string(".trip");
//	trip_info.gridsize = gridsize;
//	trip_info.n_partitions = 1;
//	//partitioning_io_output_timer.start(); // TIMING
//	writeTripHeader(header, trip_info);
//	//partitioning_io_output_timer.stop(); // TIMING
//	return trip_info;
//}
//
//// Partition the mesh referenced by tri_info into n triangle partitions for gridsize,
//// and store information about the partitioning in trip_info
//TripInfo partition(const TriInfo& tri_info, const size_t n_partitions, const size_t gridsize) {
//	// Special case: just one partition
//	if (n_partitions == 1) {
//		return partition_one(tri_info, gridsize);
//	}
//
//	// Open tri_data stream
//	//partitioning_io_input_timer.start(); // TIMING
//	//the reader knows how many triangles there are in the model,
//	// is given input_buffersize as buffersize
//	TriReader reader = TriReader(tri_info.base_filename + string(".tridata"), tri_info.n_triangles, input_buffersize);
//	//partitioning_io_input_timer.stop(); // TIMING
//
//	//partitioning_algorithm_timer.start(); // TIMING
//
//	// Create Mortonbuffers: we will have one buffer per partition
//	vector<Buffer*> buffers;
//	createBuffers(tri_info, n_partitions, gridsize, buffers);
//
//	while (reader.hasNext()) {
//		Triangle t;
//		//partitioning_algorithm_timer.stop(); partitioning_io_input_timer.start(); // TIMING
//		reader.getTriangle(t);
//		//partitioning_io_input_timer.stop(); partitioning_algorithm_timer.start(); // TIMING
//		AABox<vec3> bbox = computeBoundingBox(t.v0, t.v1, t.v2); // compute bounding box
//		for (int j = 0; j < n_partitions; j++) { // Test against all partitions
//			buffers[j]->processTriangle(t, bbox);
//		}
//	}
//	//partitioning_algorithm_timer.stop(); // TIMING
//	//partitioning_io_output_timer.start(); // TIMING
//
//	// create TripInfo object to hold header info
//	TripInfo trip_info = TripInfo(tri_info);
//
//	// Collect ntriangles and close buffers
//	trip_info.part_tricounts.resize(n_partitions);
//	for (size_t j = 0; j < n_partitions; j++) {
//		trip_info.part_tricounts[j] = buffers[j]->n_triangles;
//		delete buffers[j];
//	}
//
//	// Write trip header
//	trip_info.base_filename = tri_info.base_filename + val_to_string(gridsize) + string("_") + val_to_string(n_partitions);
//	std::string header = trip_info.base_filename + string(".trip");
//	trip_info.gridsize = gridsize;
//	trip_info.n_partitions = n_partitions;
//	writeTripHeader(header, trip_info);
//
//	//partitioning_io_output_timer.stop(); // TIMING
//	return trip_info;
//}