#include "bone_geometry.h"
#include "texture_to_render.h"
#include <fstream>
#include <iostream>
#include <glm/gtx/io.hpp>
#include <unordered_map>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
/*
 * We put these functions to a separated file because the following jumbo
 * header consists of 20K LOC, and it is very slow to compile.
 */
#include "json.hpp"

using json = nlohmann::json;

std::string save_name;
Fl_Input *save_input;
Fl_Button *save_button;
Fl_Window *window;
bool if_save = false;

void save_cb(Fl_Button *theButton, void* v) {
	
	Mesh *m = (Mesh*)v;
	save_name = save_input->value();
	//std::cout << if_save << std::endl;
	if_save = true;
	m->saveAnimationTo(save_name);
	//window->hide();
	//delete window;
}

void Mesh::saveAnimationTo(const std::string& fn)
{
	// FIXME: Save keyframes to json file.
	json json_save;
	for (size_t i = 0; i < key_frames.size(); i++) {
		json a_frame = json::array();
		json rotation = json::array();
		json translation = json::array();
		for (size_t j = 0; j < skeleton.joints.size(); j++) {
			json rot_joint = json::array();
			rot_joint.emplace_back(key_frames[i].rel_rot[j].x);
			rot_joint.emplace_back(key_frames[i].rel_rot[j].y);
			rot_joint.emplace_back(key_frames[i].rel_rot[j].z);
			rot_joint.emplace_back(key_frames[i].rel_rot[j].w);
			rotation.emplace_back(rot_joint);
		}
		translation.emplace_back(key_frames[i].root_trans.x);
		translation.emplace_back(key_frames[i].root_trans.y);
		translation.emplace_back(key_frames[i].root_trans.z);
		a_frame.emplace_back(rotation);
		a_frame.emplace_back(translation);
		json_save.emplace_back(a_frame);
	}
	std::ofstream o(fn);
	o << std::setw(4) << json_save << std::endl;
}

void Mesh::loadAnimationFrom(const std::string& fn)
{
	// FIXME: Load keyframes from json file.
	std::ifstream i(fn);
	json json_load;
	i >> json_load;
	for (size_t i = 0; i < json_load.size(); i++) {
		json a_frame = json_load[i];
		json rotation = a_frame[0];
		json translation = a_frame[1];
		KeyFrame frame;
		frame.root_trans = glm::vec3(translation[0], translation[1], translation[2]);
		for (size_t j = 0; j < rotation.size(); j++) {
			json rot_joint = rotation[j];
			glm::fquat rel_rotation = glm::fquat(rot_joint[3], rot_joint[0], rot_joint[1], rot_joint[2]);
			frame.rel_rot.emplace_back(rel_rotation);
		}
		key_frames.emplace_back(frame);
	}
	updateSkeleton(key_frames[0]);
	skeleton.refreshCache(&currentQ_);
	std::cout << "end load from json" << std::endl;

}

int Mesh::saveCustomAnimationTo() {
	std::cout << "custom save" << std::endl;
	window = new Fl_Window(500,300);
	save_input = new Fl_Input(100, 50, 200, 20, "Save Name");
	save_button = new Fl_Button(200, 100, 100, 30, "Save"); 
	save_name = save_input->value();
	save_button->callback((Fl_Callback*)save_cb, (void*)this);

	// Fl_Window *window = new Fl_Window(340,180);
	// Fl_Box *box = new Fl_Box(20,40,300,100,"Hello, World!");
	// box->box(FL_UP_BOX);
	// box->labelfont(FL_BOLD+FL_ITALIC);
	// box->labelsize(36);
	// box->labeltype(FL_SHADOW_LABEL);
	//window->end();
	window->show();
	std::cout << if_save << std::endl;
	if (if_save) {
		saveAnimationTo(save_name);
	}
	return Fl::run();
}

