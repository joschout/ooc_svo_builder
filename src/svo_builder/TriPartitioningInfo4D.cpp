#include "TriPartitioningInfo4D.h"
#include "TriInfo4D.h"
#include "TriInfo4D_multiple_files.h"

TriPartitioningInfo4D::TriPartitioningInfo4D():
	mesh_bbox_transl(AABox<vec4>()),
	base_filename(""), version(1),
	geometry_only(0), gridsize_S(0), gridsize_T(0), n_triangles(0),
	n_partitions(0)
{	
}

TriPartitioningInfo4D::TriPartitioningInfo4D(const TriInfo4D  &t):
	mesh_bbox_transl(t.mesh_bbox_transformed),
	base_filename(t.triInfo3D.base_filename),
	version(t.triInfo3D.version),
	geometry_only(t.triInfo3D.geometry_only), gridsize_S(0), gridsize_T(0), n_triangles(0),
	n_partitions(0)
{
}

TriPartitioningInfo4D::TriPartitioningInfo4D(const TriInfo4D_multiple_files& t):
	mesh_bbox_transl(t.getTotalBoundingBox()),
	base_filename(t.base_filename_without_number),
	version(t.getTriInfoVector()[0].version),
	geometry_only(t.getTriInfoVector()[0].geometry_only), gridsize_S(0), gridsize_T(0),
	n_triangles(t.getNbfOfTriangles()),
	n_partitions(0)
{
}

TriPartitioningInfo4D::~TriPartitioningInfo4D()
{
}

void TriPartitioningInfo4D::print() const
{
	
	
	cout << "  base_filename: " << base_filename << endl;
	cout << "  trip version: " << version << endl;
	cout << "  geometry only: " << geometry_only << endl;
	cout << "  gridsize_S: " << gridsize_S << endl;
	cout << "  gridsize_T: " << gridsize_T << endl;
	cout << "  bbox min: " 
		<< mesh_bbox_transl.min[0] << " " 
		<< mesh_bbox_transl.min[1] << " " 
		<< mesh_bbox_transl.min[2] << " "
		<< mesh_bbox_transl.min[3] << endl;
	cout << "  bbox max: " 
		<< mesh_bbox_transl.max[0] << " "
		<< mesh_bbox_transl.max[1] << " "
		<< mesh_bbox_transl.max[2] << " "
		<< mesh_bbox_transl.max[3] << endl;
	cout << "  n_triangles: " << n_triangles << endl;
	cout << "  n_partitions: " << n_partitions << endl;
	for (size_t i = 0; i< n_partitions; i++) {
		cout << "  partition " << i << " - tri_count: " << nbOfTrianglesPerPartition[i] << endl;
	}

}

bool TriPartitioningInfo4D::filesExist() const
{
	string header = base_filename + string(".trip");
	for (size_t i = 0; i< n_partitions; i++) {
		if (nbOfTrianglesPerPartition[i] > 0) { // we only require the file to be there if it contains any triangles.
			string part_data_filename 
				= base_filename + string("_") + val_to_string(i) + string(".tripdata");
			if (!file_exists(part_data_filename)) {
				return false;
			}
		}
	}
	return (file_exists(header));
}


