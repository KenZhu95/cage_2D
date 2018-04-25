#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <iostream>
#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <mmdadapter.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>

class TextureToRender;

struct BoundingBox {
	BoundingBox()
		: min(glm::vec3(-std::numeric_limits<float>::max())),
		max(glm::vec3(std::numeric_limits<float>::max())) {}
	glm::vec3 min;
	glm::vec3 max;
};

struct Joint {
	Joint()
		: joint_index(-1),
		  parent_index(-1),
		  position(glm::vec3(0.0f)),
		  init_position(glm::vec3(0.0f))
	{
	}
	Joint(int id, glm::vec3 wcoord, int parent)
		: joint_index(id),
		  parent_index(parent),
		  position(wcoord),
		  init_position(wcoord)
	{
	}

	int joint_index;
	int parent_index;
	glm::vec3 position;             // position of the joint
	glm::fquat orientation;         // rotation w.r.t. initial configuration
	glm::fquat rel_orientation;     // rotation w.r.t. it's parent. Used for animation.
	glm::vec3 init_position;        // initial position of this joint
	std::vector<int> children;
};

struct Configuration {
	std::vector<glm::vec3> trans;
	std::vector<glm::fquat> rot;

	const void* transData() const { return trans.data(); }
	const void* rotData() const { return rot.data(); }
};

struct KeyFrame {
	std::vector<glm::fquat> rel_rot;
	glm::vec3 root_trans;

	static void interpolate(const KeyFrame& from,
	                        const KeyFrame& to,
	                        float tau,
	                        KeyFrame& target);
	static void linear_trans(const KeyFrame& from,
	                        const KeyFrame& to,
	                        float tau,
	                        KeyFrame& target);
	static void interpolate_spline(const std::vector<KeyFrame>& key_frames, float t, KeyFrame& target);
	static glm::fquat calculateSplineQuat(const std::vector<glm::fquat>& bone_rels, float t);
	static glm::vec3 calculateSplineTrans(const std::vector<glm::vec3>& bone_trans, float t);
};

struct LineMesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::uvec2> indices;
};


struct Bone{
	Bone(Joint j_start, Joint j_end){
		start = &j_start;
		end = &j_end;
		bone_index = j_end.joint_index;
		parent_index = j_start.joint_index;
		length = glm::length(j_end.position - j_start.position);
		direction = glm::normalize(j_end.position - j_start.position);
		if (direction.x != 0){
			normal_y = glm::normalize(glm::cross(direction, glm::vec3(0.0,1.0,0.0)));
		} else {
			normal_y = glm::normalize(glm::cross(direction, glm::vec3(1.0,0.0,0.0)));
		}
		normal_z = glm::normalize(glm::cross(direction, normal_y));
		alignMatrix = glm::mat4(1.0);
		alignMatrix[0][0] = direction[0];
		alignMatrix[1][0] = direction[1];
		alignMatrix[2][0] = direction[2];
		alignMatrix[0][1] = normal_y[0];
		alignMatrix[1][1] = normal_y[1];
		alignMatrix[2][1] = normal_y[2];
		alignMatrix[0][2] = normal_z[0];
		alignMatrix[1][2] = normal_z[1];
		alignMatrix[2][2] = normal_z[2];
		inverseAlignMatrix = glm::mat4(1.0);
		inverseAlignMatrix[0][0] = direction[0];
		inverseAlignMatrix[0][1] = direction[1];
		inverseAlignMatrix[0][2] = direction[2];
		inverseAlignMatrix[1][0] = normal_y[0];
		inverseAlignMatrix[1][1] = normal_y[1];
		inverseAlignMatrix[1][2] = normal_y[2];
		inverseAlignMatrix[2][0] = normal_z[0];
		inverseAlignMatrix[2][1] = normal_z[1];
		inverseAlignMatrix[2][2] = normal_z[2];

		orientation = glm::fquat(1.0,0.0,0.0,0.0);
		rel_orientation = glm::fquat(1.0,0.0,0.0,0.0);
	}

	glm::mat4 getTranslation();

	glm::mat4 getUndeformed();

	glm::mat4 getDeformed();

	glm::fquat getOrientation();

	glm::fquat getRelOrientation();

	glm::vec4 boneToWorld(glm::vec4 bone_coor){
		return deformed_transform * bone_coor;
	}

	glm::vec4 worldToBone(glm::vec4 world_coor){
		return glm::inverse(deformed_transform) * world_coor;
	}

	glm::vec4 boneAlign(glm::vec4 bone_coor);

	glm::mat4 getCylinderTransform();

	void performRotateTotal(glm::fquat rotate_quat);
	void performRotate(glm::fquat rotate_quat);
	void performParentRotate();
	void performTranslation(glm::vec3 diff_translation);
	void performAnimationTranslation(glm::vec3 diff_translation);
	void performParentTranslation();


	int bone_index;
	int parent_index;
	int root_joint_index;
	Joint* start;
	Joint* end;
	float length;
	Bone* parent;
	std::vector<Bone*> children;
	glm::mat4 translation;               // translation matrix of bone w.r.t parent
	glm::mat4 init_translation;               // initial translation matrix of bone w.r.t parent
	glm::mat4 undeformed_transform;      // undoformed transformation matrix
	glm::mat4 deformed_transform;        // deformed transformation matrix
	glm::fquat rel_orientation;          // rotation w.r.t its parent
	glm::fquat orientation;              // rotation w.r.t. initial configuration
	glm::vec3 direction;                 // direction of bone (x axis)
	glm::vec3 normal_y;                  // normal direction (y axis)
	glm::vec3 normal_z;                  // another normal direction (z axis)
	glm::mat4 alignMatrix;               // matrix to align local coordinates along with bone direction
	glm::mat4 inverseAlignMatrix;        // matrix to transform from bone direction to initial local coordinates 
};



struct Skeleton {
	std::vector<Joint> joints;
	std::vector<Bone*> bones;
	bool if_save = false;

	Configuration cache;

	void refreshCache(Configuration* cache = nullptr);
	const glm::vec3* collectJointTrans() const;
	const glm::fquat* collectJointRot() const;

	// FIXME: create skeleton and bone data structures
	void constructBone(int joint_id);
	Bone* getBone(int bone_id);

	std::vector<glm::mat4> bone_transforms;
	const glm::mat4* collectBoneTransforms() const;
	const glm::mat4* getBoneTransform(int bone_id) const;
	void doTranslation(glm::vec3 diff_translation, int roor_id);
	void doAnimationTranslation(glm::vec3 diff_translation, int roor_id);
};

struct Vertex {
	int id;
	glm::vec2 position;
	glm::vec2 position_init;
};

struct Edge
{
	int id;
	Vertex* vertex_1;
	Vertex* vertex_2;
	glm::vec2 normal_init;
	glm::vec2 normal;
	void calculateNormal();
	void initiateNormal();
};

struct Weights {
	std::vector<float> weight_vertices;
	std::vector<float> weight_edges;
};



struct Cage {
	std::vector<Vertex> vertices;
	std::vector<Edge> edges;
	std::vector<Weights> weigtht_functions;
	std::vector<float> edge_scalings;
	void updateNormals();
	void initNormals();
	void initWeights(glm::vec2 eta);
	void updateWeights();
};

struct Mesh {
	Mesh();
	~Mesh();
	std::string save_name;
	// Fl_Input *save_input;
	// Fl_Button *save_button;


	std::vector<glm::vec4> vertices;
	/*
	 * Static per-vertex attrributes for Shaders
	 */
	std::vector<int32_t> joint0;
	std::vector<int32_t> joint1;
	std::vector<float> weight_for_joint0; // weight_for_joint1 can be calculated
	std::vector<glm::vec3> vector_from_joint0;
	std::vector<glm::vec3> vector_from_joint1;
	std::vector<glm::vec4> vertex_normals;
	std::vector<glm::vec4> face_normals;
	std::vector<glm::vec2> uv_coordinates;
	std::vector<glm::uvec3> faces;

	std::vector<KeyFrame> key_frames;
	std::vector<TextureToRender *> previews;

	std::vector<Material> materials;
	BoundingBox bounds;
	Skeleton skeleton;
	Cage cage;

	void loadPmd(const std::string& fn);
	void generateCage();
	void initCageWeights();
	void updateCageWeights();
	int getNumberOfBones() const;
	glm::vec3 getCenter() const { return 0.5f * glm::vec3(bounds.min + bounds.max); }
	const Configuration* getCurrentQ() const; // Configuration is abbreviated as Q
	void updateAnimation(float t = -1.0);
	Bone* getBone(int bone_id);
	void createKeyFrame();
	void deleteKeyFrame(int frame_id);
	void updateKeyFrame(int frame_id);
	void spaceKeyFrame(int frame_id);
	void insertKeyFrame(int frame_id);

	void updateSkeleton(KeyFrame key_frame);
	void updateFromRel(Joint& parent);

	void saveAnimationTo(const std::string& fn);
	void loadAnimationFrom(const std::string& fn);
	int saveCustomAnimationTo();

	bool isSpline() {
		return if_spline_interpolate;
	}

	void setSpline(bool is) {
		if_spline_interpolate = is;
	}


private:
	void computeBounds();
	void computeNormals();
	Configuration currentQ_;
	bool if_spline_interpolate = false;
};


#endif
