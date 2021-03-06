/*
-------------------------------------------------------------------------------
	This source file is part of Cataclysmos
	For the latest info, see http://www.cataclysmos.org/

	Copyright (c) 2005 The Cataclysmos Team
    Copyright (C) 2005  Erik Ogenvik

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
-------------------------------------------------------------------------------
*/
// Includes
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "XMLModelDefinitionSerializer.h"
#include "Model.h"
#include "components/ogre/XMLHelper.h"

#ifdef WIN32
#include <tchar.h>
#define snprintf _snprintf
#include <io.h> // for _access, Win32 version of stat()
#include <direct.h> // for _mkdir
//	#include <sys/stat.h>


	//#include <iostream>
	//#include <fstream>
	//#include <ostream>
#endif

#include "framework/osdir.h"

#include <boost/algorithm/string.hpp>

namespace Ember {
namespace OgreView {
namespace Model {


ModelDefinitionPtr XMLModelDefinitionSerializer::parseDocument(TiXmlDocument& xmlDoc, const std::string& origin) {
	TiXmlElement* rootElem = xmlDoc.RootElement();
	if (rootElem) {

		if (rootElem->ValueStr() == "model") {
			auto& name = origin;
			try {
				auto modelDef = std::make_shared<ModelDefinition>();
				if (modelDef) {
					readModel(modelDef, rootElem);
					modelDef->setValid(true);
					modelDef->setOrigin(origin);
					return modelDef;
				}
			} catch (const Ogre::Exception& ex) {
				S_LOG_FAILURE("Error when parsing model '" << name << "'." << ex);
			}
		} else {
			S_LOG_FAILURE("Invalid initial element in model definition '" << origin << "': " << rootElem->ValueStr());
		}
	}
	return ModelDefinitionPtr();
}

ModelDefinitionPtr XMLModelDefinitionSerializer::parseScript(std::istream& stream, const boost::filesystem::path& path) {
	TiXmlDocument xmlDoc;
	XMLHelper xmlHelper;
	if (xmlHelper.Load(xmlDoc, stream, path)) {
		return parseDocument(xmlDoc, path.string());
	}
	return ModelDefinitionPtr();
}

ModelDefinitionPtr XMLModelDefinitionSerializer::parseScript(Ogre::DataStreamPtr& stream) {
	TiXmlDocument xmlDoc;
	XMLHelper xmlHelper;
	if (xmlHelper.Load(xmlDoc, stream)) {
		return parseDocument(xmlDoc, stream->getName());
	}
	return ModelDefinitionPtr();
}

void XMLModelDefinitionSerializer::readModel(const ModelDefinitionPtr& modelDef, TiXmlElement* modelNode) {
	TiXmlElement* elem;
	//root elements
	//scale
	const char* tmp = modelNode->Attribute("scale");
	if (tmp)
		modelDef->mScale = std::stof(tmp);

	//showcontained
	tmp = modelNode->Attribute("showcontained");
	if (tmp) {
		modelDef->mShowContained = boost::algorithm::to_lower_copy(std::string(tmp)) == "true";
	}

	//usescaleof
	tmp = modelNode->Attribute("usescaleof");
	if (tmp) {
		std::string useScaleOf(tmp);
		if (useScaleOf == "height") {
			modelDef->mUseScaleOf = ModelDefinition::UseScaleOf::MODEL_HEIGHT;
		} else if (useScaleOf == "width") {
			modelDef->mUseScaleOf = ModelDefinition::UseScaleOf::MODEL_WIDTH;
		} else if (useScaleOf == "depth") {
			modelDef->mUseScaleOf = ModelDefinition::UseScaleOf::MODEL_DEPTH;
		} else if (useScaleOf == "none") {
			modelDef->mUseScaleOf = ModelDefinition::UseScaleOf::MODEL_NONE;
		} else if (useScaleOf == "fit") {
			modelDef->mUseScaleOf = ModelDefinition::UseScaleOf::MODEL_FIT;
		} else {
			S_LOG_WARNING("Unrecognized model scaling directive: " << useScaleOf);
		}
	}

	tmp = modelNode->Attribute("renderingdistance");
	if (tmp) {
		modelDef->setRenderingDistance(std::stof(tmp));
	}

	tmp = modelNode->Attribute("icon");
	if (tmp) {
		modelDef->mIconPath = tmp;
	}

	tmp = modelNode->Attribute("useinstancing");
	if (tmp) {
		modelDef->mUseInstancing = modelDef->mShowContained = boost::algorithm::to_lower_copy(std::string(tmp)) == "true";
	}

	//submodels
	elem = modelNode->FirstChildElement("submodels");
	if (elem)
		readSubModels(modelDef, elem);

	//actions
	elem = modelNode->FirstChildElement("actions");
	if (elem)
		readActions(modelDef, elem);

	//attachpoints
	elem = modelNode->FirstChildElement("attachpoints");
	if (elem)
		readAttachPoints(modelDef, elem);

	//attachpoints
	elem = modelNode->FirstChildElement("particlesystems");
	if (elem)
		readParticleSystems(modelDef, elem);

	//views
	elem = modelNode->FirstChildElement("views");
	if (elem)
		readViews(modelDef, elem);

	elem = modelNode->FirstChildElement("translate");
	if (elem) {
		modelDef->mTranslate = XMLHelper::fillVector3FromElement(elem);
	}

	elem = modelNode->FirstChildElement("rotation");
	if (elem) {
		modelDef->setRotation(XMLHelper::fillQuaternionFromElement(elem));
	}

	elem = modelNode->FirstChildElement("contentoffset");
	if (elem) {
		Ogre::Vector3 offset = XMLHelper::fillVector3FromElement(elem);
		modelDef->setContentOffset(offset);
	}

	elem = modelNode->FirstChildElement("rendering");
	if (elem) {
		modelDef->mRenderingDef = new RenderingDefinition();
		tmp = elem->Attribute("scheme");
		if (tmp) {
			modelDef->mRenderingDef->setScheme(tmp);
		}
		for (TiXmlElement* paramElem = elem->FirstChildElement("param");
			 paramElem != nullptr; paramElem = paramElem->NextSiblingElement()) {
			tmp = paramElem->Attribute("key");
			if (tmp) {
				modelDef->mRenderingDef->mParams.insert(StringParamStore::value_type(tmp, paramElem->GetText()));
			}
		}
	}

	elem = modelNode->FirstChildElement("lights");
	if (elem)
		readLights(modelDef, elem);

	elem = modelNode->FirstChildElement("bonegroups");
	if (elem)
		readBoneGroups(modelDef, elem);
	elem = modelNode->FirstChildElement("poses");
	if (elem)
		readPoses(modelDef, elem);

}


void XMLModelDefinitionSerializer::readSubModels(const ModelDefinitionPtr& modelDef, TiXmlElement* mSubModelNode) {
	S_LOG_VERBOSE("Read Submodels");
	const char* tmp = nullptr;
	TiXmlElement* elem;
	bool notfound = true;

	for (TiXmlElement* smElem = mSubModelNode->FirstChildElement();
		 smElem != nullptr; smElem = smElem->NextSiblingElement()) {
		notfound = false;

		tmp = smElem->Attribute("mesh");
		if (tmp) {
			SubModelDefinition* subModelDef = modelDef->createSubModelDefinition(tmp);

			tmp = smElem->Attribute("shadowcaster");
			if (tmp) {
				subModelDef->mShadowCaster = boost::algorithm::to_lower_copy(std::string(tmp)) == "true";
			}

			S_LOG_VERBOSE(" Add submodel  : " + subModelDef->getMeshName());
			try {
				//preload
				//FIX Ogre::MeshManager::getSingleton().load(subModelDef.Mesh);
/*					Ogre::MeshManager::getSingleton().load( subModelDef.Mesh,
							Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );*/

				//parts

				elem = smElem->FirstChildElement("parts");
				if (elem)
					readParts(elem, subModelDef);

//				modelDef->mSubModels.push_back(subModelDef);

			}
			catch (const Ogre::Exception& e) {
				S_LOG_FAILURE("Load error : " << tmp);
			}
		}
	}

	if (notfound) {
		S_LOG_VERBOSE("No submodel found.");
	}
}

void XMLModelDefinitionSerializer::readParts(TiXmlElement* mPartNode, SubModelDefinition* def) {
	TiXmlElement* elem;
	const char* tmp = nullptr;
	bool notfound = true;

	for (TiXmlElement* partElem = mPartNode->FirstChildElement();
		 partElem != nullptr; partElem = partElem->NextSiblingElement()) {

		// name
		tmp = partElem->Attribute("name");
		if (tmp) {
			notfound = false;

			PartDefinition* partDef = def->createPartDefinition(tmp);

			S_LOG_VERBOSE("  Add part  : " + partDef->getName());

			// show
			tmp = partElem->Attribute("show");
			if (tmp)
				partDef->setShow(boost::algorithm::to_lower_copy(std::string(tmp)) == "true");

			tmp = partElem->Attribute("group");
			if (tmp)
				partDef->setGroup(tmp);

			elem = partElem->FirstChildElement("subentities");
			if (elem)
				readSubEntities(elem, partDef);

		} else {
			S_LOG_FAILURE("A name must be specified for each part.");
		}
	}

	if (notfound) {
		S_LOG_VERBOSE("No part found.");
	}
}

void XMLModelDefinitionSerializer::readSubEntities(TiXmlElement* mSubEntNode, PartDefinition* def) {

	const char* tmp = nullptr;
	bool notfound = true;

	for (TiXmlElement* seElem = mSubEntNode->FirstChildElement();
		 seElem != nullptr; seElem = seElem->NextSiblingElement()) {
		SubEntityDefinition* subEntityDef = nullptr;
		// name
		tmp = seElem->Attribute("index");
		if (tmp) {
			notfound = false;
			subEntityDef = def->createSubEntityDefinition(static_cast<unsigned int>(std::strtoul(tmp, nullptr, 10)));
			S_LOG_VERBOSE("   Add sub entity with index: " << subEntityDef->getSubEntityIndex());
		} else {
			tmp = seElem->Attribute("name");
			if (tmp) {
				notfound = false;
				subEntityDef = def->createSubEntityDefinition(tmp);
				S_LOG_VERBOSE("   Add sub entity: " << subEntityDef->getSubEntityName());
			}
		}
		if (!notfound) {
			//material
			tmp = seElem->Attribute("material");
			if (tmp) {
				subEntityDef->setMaterialName(tmp);
				//preload subEntityDef.Material
				//Ogre::MaterialManager::getSingleton().load( subEntityDef.Material,
				//Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

			}

		} else {
			S_LOG_FAILURE("A subentity name or index must be specified for each subentity.");
		}
	}

	if (notfound) {
		S_LOG_VERBOSE("No sub entity found.");
	}
}

void XMLModelDefinitionSerializer::readActions(ModelDefinitionPtr modelDef, TiXmlElement* mAnimNode) {
	TiXmlElement* elem;
	const char* tmp = nullptr;
	bool notfound = true;
	S_LOG_VERBOSE("Read Actions");

	for (TiXmlElement* animElem = mAnimNode->FirstChildElement();
		 animElem != nullptr; animElem = animElem->NextSiblingElement()) {
		notfound = false;

		tmp = animElem->Attribute("name");
		if (tmp) {
			ActionDefinition* actionDef = modelDef->createActionDefinition(tmp);
			S_LOG_VERBOSE(" Add action  : " << tmp);

			tmp = animElem->Attribute("speed");
			if (tmp) {
				actionDef->setAnimationSpeed(std::stof(tmp));
			}


			elem = animElem->FirstChildElement("animations");
			if (elem)
				readAnimations(elem, actionDef);

			elem = animElem->FirstChildElement("sounds");
			if (elem)
				readSounds(elem, actionDef);

			elem = animElem->FirstChildElement("activations");
			if (elem)
				readActivations(elem, actionDef);
		}

	}

	if (notfound) {
		S_LOG_VERBOSE("No actions found.");
	}

}

void XMLModelDefinitionSerializer::readSounds(TiXmlElement* mAnimationsNode, ActionDefinition* action) {
	const char* tmp = nullptr;

	for (TiXmlElement* soundElem = mAnimationsNode->FirstChildElement();
		 soundElem != nullptr; soundElem = soundElem->NextSiblingElement()) {
		tmp = soundElem->Attribute("group");
		if (tmp) {
			std::string groupName(tmp);

			unsigned int playOrder = 0;
			tmp = soundElem->Attribute("playOrder");
			if (tmp) {
				std::string playO(tmp);
				if (playO == "linear")
					playOrder = 0;
				else if (playO == "inverse")
					playOrder = 1;
				else if (playO == "random")
					playOrder = 2;
			}

			action->createSoundDefinition(groupName, playOrder);
			S_LOG_VERBOSE("  Add Sound: " << groupName);
		}
	}
}

void XMLModelDefinitionSerializer::readActivations(TiXmlElement* activationsNode, ActionDefinition* action) {
	const char* tmp = nullptr;

	for (TiXmlElement* activationElem = activationsNode->FirstChildElement();
		 activationElem != nullptr; activationElem = activationElem->NextSiblingElement()) {
		tmp = activationElem->Attribute("type");
		if (tmp) {
			std::string typeString(tmp);

			ActivationDefinition::Type type;
			if (typeString == "movement") {
				type = ActivationDefinition::MOVEMENT;
			} else if (typeString == "action") {
				type = ActivationDefinition::ACTION;
			} else if (typeString == "task") {
				type = ActivationDefinition::TASK;
			} else {
				S_LOG_WARNING("No recognized activation type: " << typeString);
				continue;
			}
			std::string trigger = activationElem->GetText();
			action->createActivationDefinition(type, trigger);
			S_LOG_VERBOSE("  Add activation: " << typeString << " : " << trigger);
		}
	}
}

void XMLModelDefinitionSerializer::readAnimations(TiXmlElement* mAnimationsNode, ActionDefinition* action) {
	const char* tmp = nullptr;
	bool nopartfound = true;


	for (TiXmlElement* animElem = mAnimationsNode->FirstChildElement();
		 animElem != nullptr; animElem = animElem->NextSiblingElement()) {
		int iterations(1);
		nopartfound = false;

		// name
		tmp = animElem->Attribute("iterations");
		if (tmp) {
			iterations = std::stoi(tmp);
		}

		AnimationDefinition* animDef = action->createAnimationDefinition(iterations);
		readAnimationParts(animElem, animDef);
	}

	if (nopartfound) {
		S_LOG_VERBOSE("  No animations found!!");
	}

}

void XMLModelDefinitionSerializer::readAnimationParts(TiXmlElement* mAnimPartNode, AnimationDefinition* animDef) {
	const char* tmp = nullptr;
	bool nopartfound = true;

	for (TiXmlElement* apElem = mAnimPartNode->FirstChildElement();
		 apElem != nullptr; apElem = apElem->NextSiblingElement()) {
		std::string name;
		nopartfound = false;

		// name
		tmp = apElem->Attribute("name");
		if (tmp) {
			name = tmp;
			S_LOG_VERBOSE("  Add animation  : " + name);
		}

		AnimationPartDefinition* animPartDef = animDef->createAnimationPartDefinition(name);

		TiXmlElement* elem = apElem->FirstChildElement("bonegrouprefs");
		if (elem) {
			for (TiXmlElement* boneGroupRefElem = elem->FirstChildElement();
				 boneGroupRefElem != nullptr; boneGroupRefElem = boneGroupRefElem->NextSiblingElement()) {
				tmp = boneGroupRefElem->Attribute("name");
				if (tmp) {
					BoneGroupRefDefinition boneGroupRef;
					boneGroupRef.Name = tmp;
					tmp = boneGroupRefElem->Attribute("weight");
					if (tmp) {
						boneGroupRef.Weight = std::stof(tmp);
					} else {
						boneGroupRef.Weight = 1.0f;
					}
					animPartDef->BoneGroupRefs.push_back(boneGroupRef);
				}
			}
		}


	}

	if (nopartfound) {
		S_LOG_VERBOSE("   No anim parts found.");
	}
}


void XMLModelDefinitionSerializer::readAttachPoints(ModelDefinitionPtr modelDef, TiXmlElement* mAnimPartNode) {
	AttachPointDefinitionStore& attachPoints = modelDef->mAttachPoints;

	const char* tmp = nullptr;

	for (TiXmlElement* apElem = mAnimPartNode->FirstChildElement();
		 apElem != nullptr; apElem = apElem->NextSiblingElement()) {
		AttachPointDefinition attachPointDef;

		// name
		tmp = apElem->Attribute("name");
		if (tmp)
			attachPointDef.Name = tmp;
		S_LOG_VERBOSE("  Add attachpoint  : " + attachPointDef.Name);

		// bone
		tmp = apElem->Attribute("bone");
		if (tmp)
			attachPointDef.BoneName = tmp;

		// pose
		tmp = apElem->Attribute("pose");
		if (tmp)
			attachPointDef.Pose = tmp;

		TiXmlElement* elem = apElem->FirstChildElement("rotation");
		if (elem) {
			attachPointDef.Rotation = XMLHelper::fillQuaternionFromElement(elem);
		} else {
			attachPointDef.Rotation = Ogre::Quaternion::IDENTITY;
		}

		TiXmlElement* tranelem = apElem->FirstChildElement("translation");
		if (tranelem) {
			attachPointDef.Translation = XMLHelper::fillVector3FromElement(tranelem);
		} else {
			attachPointDef.Translation = Ogre::Vector3::ZERO;
		}

		attachPoints.push_back(attachPointDef);
	}

}

void XMLModelDefinitionSerializer::readParticleSystems(ModelDefinitionPtr modelDef, TiXmlElement* mParticleSystemsNode) {
	TiXmlElement* elem;
	ModelDefinition::ParticleSystemSet& particleSystems = modelDef->mParticleSystems;

	const char* tmp = nullptr;

	for (TiXmlElement* apElem = mParticleSystemsNode->FirstChildElement();
		 apElem != nullptr; apElem = apElem->NextSiblingElement()) {
		ModelDefinition::ParticleSystemDefinition def;

		// name
		tmp = apElem->Attribute("script");
		if (tmp)
			def.Script = tmp;
		S_LOG_VERBOSE("  Add particlescript  : " + def.Script);

		elem = apElem->FirstChildElement("bindings");
		if (elem)
			readParticleSystemsBindings(def, elem);

		elem = apElem->FirstChildElement("direction");
		if (elem) {
			def.Direction = XMLHelper::fillVector3FromElement(elem);
		} else {
			def.Direction = Ogre::Vector3(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
		}


		particleSystems.push_back(def);
	}
}

void XMLModelDefinitionSerializer::readParticleSystemsBindings(ModelDefinition::ParticleSystemDefinition& def, TiXmlElement* mParticleSystemsNode) {
	const char* tmp = nullptr;
// 	bool nopartfound = true;

	for (TiXmlElement* apElem = mParticleSystemsNode->FirstChildElement();
		 apElem != nullptr; apElem = apElem->NextSiblingElement()) {
		ModelDefinition::BindingDefinition binding;

		// emittervar
		tmp = apElem->Attribute("emittervar");
		if (tmp)
			binding.EmitterVar = tmp;
		else
			continue;

		// atlasattribute
		tmp = apElem->Attribute("atlasattribute");
		if (tmp)
			binding.AtlasAttribute = tmp;
		else
			continue;

		S_LOG_VERBOSE("  Add binding between " << binding.EmitterVar << " and " << binding.AtlasAttribute << ".");


		def.Bindings.push_back(binding);
	}

}

void XMLModelDefinitionSerializer::readViews(ModelDefinitionPtr modelDef, TiXmlElement* viewsNode) {
	TiXmlElement* elem;

	const char* tmp = nullptr;

	for (TiXmlElement* viewElem = viewsNode->FirstChildElement();
		 viewElem != nullptr; viewElem = viewElem->NextSiblingElement()) {

		// name
		tmp = viewElem->Attribute("name");
		if (tmp) {
			std::string name(tmp);

			ViewDefinition* def = modelDef->createViewDefinition(name);

			S_LOG_VERBOSE(" Add View  : " + def->Name);

			elem = viewElem->FirstChildElement("rotation");
			if (elem) {
				def->Rotation = XMLHelper::fillQuaternionFromElement(elem);
			} else {
				def->Rotation = Ogre::Quaternion::IDENTITY;
			}

			elem = viewElem->FirstChildElement("distance");
			if (elem) {
				def->Distance = std::stof(elem->GetText());
			} else {
				def->Distance = 0;
			}

		}
	}
}

void XMLModelDefinitionSerializer::readLights(ModelDefinitionPtr modelDef, TiXmlElement* mLightsNode) {
	TiXmlElement* elem;
	ModelDefinition::LightSet& lights = modelDef->mLights;

	const char* tmp = nullptr;

	for (TiXmlElement* lElem = mLightsNode->FirstChildElement();
		 lElem != nullptr; lElem = lElem->NextSiblingElement()) {
		ModelDefinition::LightDefinition def;

		def.type = Ogre::Light::LT_POINT;

		tmp = lElem->Attribute("type");
		if (tmp) {
			std::string type = tmp;
			if (type == "point") {
				def.type = Ogre::Light::LT_POINT;
			} else if (type == "directional") {
				def.type = Ogre::Light::LT_DIRECTIONAL;
			} else if (type == "spotlight") {
				def.type = Ogre::Light::LT_SPOTLIGHT;
			}
		}

		Ogre::Real r = 1.0f, g = 1.0f, b = 1.0f;

		elem = lElem->FirstChildElement("diffusecolour");
		if (elem) {
			if (elem->Attribute("r")) {
				r = std::stof(elem->Attribute("r"));
			}
			if (elem->Attribute("g")) {
				g = std::stof(elem->Attribute("g"));
			}
			if (elem->Attribute("b")) {
				b = std::stof(elem->Attribute("b"));
			}
		}
		def.diffuseColour = Ogre::ColourValue(r, g, b);

		elem = lElem->FirstChildElement("specularcolour");
		if (elem) {
			if (elem->Attribute("r")) {
				r = std::stof(elem->Attribute("r"));
			}
			if (elem->Attribute("g")) {
				g = std::stof(elem->Attribute("g"));
			}
			if (elem->Attribute("b")) {
				b = std::stof(elem->Attribute("b"));
			}
			def.specularColour = Ogre::ColourValue(r, g, b);
		} else {
			def.specularColour = def.diffuseColour;
		}

		elem = lElem->FirstChildElement("attenuation");
		def.range = 100000.0;
		def.constant = 1.0;
		def.linear = 0.0;
		def.quadratic = 0.0;
		if (elem) {
			if (elem->Attribute("range")) {
				def.range = std::stof(elem->Attribute("range"));
			}
			if (elem->Attribute("constant")) {
				def.constant = std::stof(elem->Attribute("constant"));
			}
			if (elem->Attribute("linear")) {
				def.linear = std::stof(elem->Attribute("linear"));
			}
			if (elem->Attribute("quadratic")) {
				def.quadratic = std::stof(elem->Attribute("quadratic"));
			}
		}

		elem = lElem->FirstChildElement("position");
		if (elem) {
			def.position = XMLHelper::fillVector3FromElement(elem);
		} else {
			def.position = Ogre::Vector3::ZERO;
		}

		S_LOG_VERBOSE("  Add light");

		lights.push_back(def);
	}
}

void XMLModelDefinitionSerializer::readBoneGroups(ModelDefinitionPtr modelDef, TiXmlElement* boneGroupsNode) {
	TiXmlElement* elem;

	const char* tmp = nullptr;

	for (TiXmlElement* boneGroupElem = boneGroupsNode->FirstChildElement();
		 boneGroupElem != nullptr; boneGroupElem = boneGroupElem->NextSiblingElement()) {

		// name
		tmp = boneGroupElem->Attribute("name");
		if (tmp) {
			std::string name(tmp);

			BoneGroupDefinition* def = modelDef->createBoneGroupDefinition(name);

			S_LOG_VERBOSE(" Add Bone Group  : " + def->Name);

			elem = boneGroupElem->FirstChildElement("bones");
			if (elem) {
				for (TiXmlElement* boneElem = elem->FirstChildElement();
					 boneElem != nullptr; boneElem = boneElem->NextSiblingElement()) {
					const char* text = boneElem->Attribute("index");
					if (text) {
						std::istringstream stream(text);
						size_t index;
						stream >> index;
						def->Bones.push_back(index);
					}
				}
			}
		}
	}
}

void XMLModelDefinitionSerializer::readPoses(ModelDefinitionPtr modelDef, TiXmlElement* mNode) {
	PoseDefinitionStore& poses = modelDef->mPoseDefinitions;

	const char* tmp = nullptr;

	for (TiXmlElement* apElem = mNode->FirstChildElement(); apElem != nullptr; apElem = apElem->NextSiblingElement()) {
		PoseDefinition definition;

		// name
		tmp = apElem->Attribute("name");
		if (!tmp) {
			S_LOG_WARNING("Read pose definition with no name; skipping it.");
			continue;
		}
		std::string name(tmp);
		S_LOG_VERBOSE("  Add pose  : " + name);

		tmp = apElem->Attribute("ignoreEntityData");
		definition.IgnoreEntityData = false;
		if (tmp) {
			if (std::string(tmp) == "true") {
				definition.IgnoreEntityData = true;
			}
		}


		TiXmlElement* elem = apElem->FirstChildElement("rotate");
		if (elem) {
			definition.Rotate = XMLHelper::fillQuaternionFromElement(elem);
		} else {
			definition.Rotate = Ogre::Quaternion::IDENTITY;
		}

		elem = apElem->FirstChildElement("translate");
		if (elem) {
			definition.Translate = XMLHelper::fillVector3FromElement(elem);
		} else {
			definition.Translate = Ogre::Vector3::ZERO;
		}

		poses.insert(std::make_pair(name, definition));
	}

}


bool XMLModelDefinitionSerializer::exportScript(ModelDefinitionPtr modelDef, const std::string& directory, const std::string& filename) {
	if (filename.empty()) {
		return false;
	}

	TiXmlDocument xmlDoc;

	try {

		if (!oslink::directory(directory).isExisting()) {
			S_LOG_INFO("Creating directory " << directory);
			oslink::directory::mkdir(directory.c_str());
		}

		TiXmlElement elem("models");
		TiXmlElement modelElem("model");

		std::string useScaleOf;
		switch (modelDef->getUseScaleOf()) {
			case ModelDefinition::UseScaleOf::MODEL_ALL:
				useScaleOf = "all";
				break;
			case ModelDefinition::UseScaleOf::MODEL_DEPTH:
				useScaleOf = "depth";
				break;
			case ModelDefinition::UseScaleOf::MODEL_HEIGHT:
				useScaleOf = "height";
				break;
			case ModelDefinition::UseScaleOf::MODEL_NONE:
				useScaleOf = "none";
				break;
			case ModelDefinition::UseScaleOf::MODEL_WIDTH:
				useScaleOf = "width";
				break;
			case ModelDefinition::UseScaleOf::MODEL_FIT:
				useScaleOf = "fit";
				break;
		}
		modelElem.SetAttribute("usescaleof", useScaleOf);

		if (!modelDef->mUseInstancing) {
			modelElem.SetAttribute("useinstancing", "false");
		}

		if (modelDef->getRenderingDistance() != 0.0f) {
			modelElem.SetDoubleAttribute("renderingdistance", modelDef->getRenderingDistance());
		}

		if (modelDef->getScale() != 0) {
			modelElem.SetDoubleAttribute("scale", modelDef->getScale());
		}

		modelElem.SetAttribute("showcontained", modelDef->getShowContained() ? "true" : "false");

		if (modelDef->getContentOffset() != Ogre::Vector3::ZERO) {
			TiXmlElement contentOffset("contentoffset");
			XMLHelper::fillElementFromVector3(contentOffset, modelDef->getContentOffset());
			modelElem.InsertEndChild(contentOffset);
		}

		const RenderingDefinition* renderingDef = modelDef->getRenderingDefinition();
		if (renderingDef) {
			TiXmlElement rendering("rendering");
			rendering.SetAttribute("scheme", renderingDef->getScheme());
			for (const auto& aParam : renderingDef->getParameters()) {
				TiXmlElement param("param");
				param.SetAttribute("key", aParam.first);
				param.SetValue(aParam.second);
				rendering.InsertEndChild(param);
			}
			modelElem.InsertEndChild(rendering);
		}


		TiXmlElement translate("translate");
		XMLHelper::fillElementFromVector3(translate, modelDef->getTranslate());
		modelElem.InsertEndChild(translate);

		TiXmlElement rotation("rotation");
		XMLHelper::fillElementFromQuaternion(rotation, modelDef->getRotation());
		modelElem.InsertEndChild(rotation);

		modelElem.SetAttribute("icon", modelDef->getIconPath());

		if (modelDef->getRenderingDefinition()) {
			TiXmlElement rendering("rendering");
			rendering.SetAttribute("scheme", modelDef->getRenderingDefinition()->getScheme());
			for (const auto& aParam : modelDef->getRenderingDefinition()->getParameters()) {
				TiXmlElement param("param");
				param.SetAttribute("key", aParam.first);
				param.SetValue(aParam.second);
			}
		}

		//start with submodels
		exportSubModels(modelDef, modelElem);

		//now do actions
		exportActions(modelDef, modelElem);

		exportAttachPoints(modelDef, modelElem);

		exportViews(modelDef, modelElem);

		exportLights(modelDef, modelElem);

		exportParticleSystems(modelDef, modelElem);

		exportBoneGroups(modelDef, modelElem);

		exportPoses(modelDef, modelElem);

		elem.InsertEndChild(modelElem);

		xmlDoc.InsertEndChild(elem);
		xmlDoc.SaveFile((directory + filename));
		S_LOG_INFO("Saved file " << (directory + filename));
		return true;
	}
	catch (...) {
		S_LOG_FAILURE("An error occurred saving the modeldefinition for " << filename << ".");
		return false;
	}


}

void XMLModelDefinitionSerializer::exportViews(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement viewsElem("views");

	for (const auto& viewDefinition : modelDef->getViewDefinitions()) {
		TiXmlElement viewElem("view");
		viewElem.SetAttribute("name", viewDefinition.second->Name);

		TiXmlElement distanceElem("distance");
		std::stringstream ss;
		ss << viewDefinition.second->Distance;
		distanceElem.InsertEndChild(TiXmlText(ss.str()));
		viewElem.InsertEndChild(distanceElem);

		TiXmlElement rotation("rotation");
		XMLHelper::fillElementFromQuaternion(rotation, viewDefinition.second->Rotation);
		viewElem.InsertEndChild(rotation);

		viewsElem.InsertEndChild(viewElem);
	}
	modelElem.InsertEndChild(viewsElem);
}

void XMLModelDefinitionSerializer::exportActions(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement actionsElem("actions");

	for (ActionDefinitionsStore::const_iterator I = modelDef->getActionDefinitions().begin(); I != modelDef->getActionDefinitions().end(); ++I) {
		TiXmlElement actionElem("action");
		actionElem.SetAttribute("name", (*I)->getName());
		actionElem.SetDoubleAttribute("speed", (*I)->getAnimationSpeed());


		TiXmlElement activationsElem("activations");
		for (auto& activationDef : (*I)->getActivationDefinitions()) {
			TiXmlElement activationElem("activation");
			std::string type;
			switch (activationDef.type) {
				case ActivationDefinition::MOVEMENT:
					type = "movement";
					break;
				case ActivationDefinition::ACTION:
					type = "action";
					break;
				case ActivationDefinition::TASK:
					type = "task";
					break;
			}
			activationElem.SetAttribute("type", type);
			activationElem.InsertEndChild(TiXmlText(activationDef.trigger));
			activationsElem.InsertEndChild(activationElem);
		}
		actionElem.InsertEndChild(activationsElem);

		if (!(*I)->getAnimationDefinitions().empty()) {
			TiXmlElement animationsElem("animations");
			for (AnimationDefinitionsStore::const_iterator J = (*I)->getAnimationDefinitions().begin(); J != (*I)->getAnimationDefinitions().end(); ++J) {
				TiXmlElement animationElem("animation");
				animationElem.SetAttribute("iterations", (*J)->getIterations());

				for (auto animationPartDefinition : (*J)->getAnimationPartDefinitions()) {
					TiXmlElement animationPartElem("animationpart");
					animationPartElem.SetAttribute("name", animationPartDefinition->Name);
					for (std::vector<BoneGroupRefDefinition>::const_iterator L = animationPartDefinition->BoneGroupRefs.begin(); L != animationPartDefinition->BoneGroupRefs.end(); ++L) {
						TiXmlElement boneGroupRefElem("bonegroupref");
						boneGroupRefElem.SetAttribute("name", L->Name);
						if (L->Weight != 1.0f) {
							boneGroupRefElem.SetAttribute("weight", static_cast<int>(L->Weight));
						}
						animationPartElem.InsertEndChild(boneGroupRefElem);
					}
					animationElem.InsertEndChild(animationPartElem);
				}

				animationsElem.InsertEndChild(animationElem);
			}
			actionElem.InsertEndChild(animationsElem);
		}

		//for now, only allow one sound
		if (!(*I)->getSoundDefinitions().empty()) {
			TiXmlElement soundsElem("sounds");

			for (SoundDefinitionsStore::const_iterator J = (*I)->getSoundDefinitions().begin(); J != (*I)->getSoundDefinitions().end(); ++J) {
				TiXmlElement soundElem("sound");
				soundElem.SetAttribute("groupName", (*J)->groupName);
				soundElem.SetAttribute("playOrder", (*J)->playOrder);
				soundsElem.InsertEndChild(soundElem);
			}
		}
		actionsElem.InsertEndChild(actionElem);
	}
	modelElem.InsertEndChild(actionsElem);
}

void XMLModelDefinitionSerializer::exportSubModels(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement submodelsElem("submodels");

	for (const auto& subModelDefinition : modelDef->getSubModelDefinitions()) {
		TiXmlElement submodelElem("submodel");
		submodelElem.SetAttribute("mesh", subModelDefinition->getMeshName());
		if (!subModelDefinition->mShadowCaster) {
			submodelElem.SetAttribute("shadowcaster", "false");
		}
		TiXmlElement partsElem("parts");

		for (const auto& partDefinition : subModelDefinition->getPartDefinitions()) {
			TiXmlElement partElem("part");
			partElem.SetAttribute("name", partDefinition->getName());
			if (!partDefinition->getGroup().empty()) {
				partElem.SetAttribute("group", partDefinition->getGroup());
			}
			partElem.SetAttribute("show", partDefinition->getShow() ? "true" : "false");

			if (!partDefinition->getSubEntityDefinitions().empty()) {
				TiXmlElement subentitiesElem("subentities");
				for (const auto& subEntityDefinition : partDefinition->getSubEntityDefinitions()) {
					TiXmlElement subentityElem("subentity");
					if (!subEntityDefinition->getSubEntityName().empty()) {
						subentityElem.SetAttribute("name", subEntityDefinition->getSubEntityName());
					} else {
						subentityElem.SetAttribute("index", subEntityDefinition->getSubEntityIndex());
					}
					if (!subEntityDefinition->getMaterialName().empty()) {
						subentityElem.SetAttribute("material", subEntityDefinition->getMaterialName());
					}
					subentitiesElem.InsertEndChild(subentityElem);
				}
				partElem.InsertEndChild(subentitiesElem);
			}
			partsElem.InsertEndChild(partElem);
		}
		submodelElem.InsertEndChild(partsElem);
		submodelsElem.InsertEndChild(submodelElem);
	}
	modelElem.InsertEndChild(submodelsElem);

}

void XMLModelDefinitionSerializer::exportAttachPoints(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement attachpointsElem("attachpoints");

	for (const auto& attachPointDef : modelDef->getAttachPointsDefinitions()) {
		TiXmlElement attachpointElem("attachpoint");
		attachpointElem.SetAttribute("name", attachPointDef.Name);
		attachpointElem.SetAttribute("bone", attachPointDef.BoneName);
		if (!attachPointDef.Pose.empty()) {
			attachpointElem.SetAttribute("pose", attachPointDef.Pose);
		}
		TiXmlElement rotationElem("rotation");
		XMLHelper::fillElementFromQuaternion(rotationElem, attachPointDef.Rotation);
		attachpointElem.InsertEndChild(rotationElem);
		TiXmlElement translationElem("translation");
		XMLHelper::fillElementFromVector3(translationElem, attachPointDef.Translation);
		attachpointElem.InsertEndChild(translationElem);

		attachpointsElem.InsertEndChild(attachpointElem);
	}
	modelElem.InsertEndChild(attachpointsElem);
}

void XMLModelDefinitionSerializer::exportLights(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement lightsElem("lights");

	for (ModelDefinition::LightSet::const_iterator I = modelDef->mLights.begin(); I != modelDef->mLights.end(); ++I) {
		ModelDefinition::LightDefinition def(*I);
		TiXmlElement lightElem("light");
		std::string type;
		if (def.type == Ogre::Light::LT_POINT) {
			type = "point";
		} else if (def.type == Ogre::Light::LT_DIRECTIONAL) {
			type = "directional";
		} else if (def.type == Ogre::Light::LT_SPOTLIGHT) {
			type = "spotlight";
		}
		lightElem.SetAttribute("type", type);

		TiXmlElement diffuseElem("diffusecolour");
		diffuseElem.SetDoubleAttribute("r", def.diffuseColour.r);
		diffuseElem.SetDoubleAttribute("g", def.diffuseColour.g);
		diffuseElem.SetDoubleAttribute("b", def.diffuseColour.b);
		lightElem.InsertEndChild(diffuseElem);

		TiXmlElement specularElem("specularcolour");
		specularElem.SetDoubleAttribute("r", def.specularColour.r);
		specularElem.SetDoubleAttribute("g", def.specularColour.g);
		specularElem.SetDoubleAttribute("b", def.specularColour.b);
		lightElem.InsertEndChild(specularElem);

		TiXmlElement attenuationElem("attenuation");
		attenuationElem.SetDoubleAttribute("range", def.range);
		attenuationElem.SetDoubleAttribute("constant", def.constant);
		attenuationElem.SetDoubleAttribute("linear", def.linear);
		attenuationElem.SetDoubleAttribute("quadratic", def.quadratic);
		lightElem.InsertEndChild(attenuationElem);

		TiXmlElement posElem("position");
		XMLHelper::fillElementFromVector3(posElem, def.position);
		lightElem.InsertEndChild(posElem);

		lightsElem.InsertEndChild(lightElem);
	}
	modelElem.InsertEndChild(lightsElem);
}

void XMLModelDefinitionSerializer::exportPoses(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	if (!modelDef->mPoseDefinitions.empty()) {
		TiXmlElement elem("poses");

		for (PoseDefinitionStore::const_iterator I = modelDef->mPoseDefinitions.begin(); I != modelDef->mPoseDefinitions.end(); ++I) {
			TiXmlElement poseElem("pose");
			poseElem.SetAttribute("name", I->first);
			if (I->second.IgnoreEntityData) {
				poseElem.SetAttribute("ignoreEntityData", "true");
			}
			if (!I->second.Translate.isNaN()) {
				TiXmlElement translateElem("translate");
				XMLHelper::fillElementFromVector3(translateElem, I->second.Translate);
				poseElem.InsertEndChild(translateElem);
			}
			if (!I->second.Rotate.isNaN()) {
				TiXmlElement rotateElem("rotate");
				XMLHelper::fillElementFromQuaternion(rotateElem, I->second.Rotate);
				poseElem.InsertEndChild(rotateElem);
			}

			elem.InsertEndChild(poseElem);
		}
		modelElem.InsertEndChild(elem);
	}
}

void XMLModelDefinitionSerializer::exportParticleSystems(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	if (!modelDef->mParticleSystems.empty()) {
		TiXmlElement particleSystemsElem("particlesystems");

		for (const auto& particleDef :modelDef->mParticleSystems) {
			TiXmlElement particleSystemElem("particlesystem");
			particleSystemElem.SetAttribute("script", particleDef.Script);
			if (!particleDef.Direction.isNaN()) {
				TiXmlElement directionElem("direction");
				XMLHelper::fillElementFromVector3(directionElem, particleDef.Direction);
				particleSystemElem.InsertEndChild(directionElem);
			}
			if (!particleDef.Bindings.empty()) {
				TiXmlElement bindingsElem("bindings");

				for (const auto& binding : particleDef.Bindings) {
					TiXmlElement bindingElem("binding");
					bindingsElem.SetAttribute("emittervar", binding.EmitterVar);
					bindingsElem.SetAttribute("atlasattribute", binding.AtlasAttribute);
					particleSystemElem.InsertEndChild(bindingsElem);
				}
			}
			particleSystemsElem.InsertEndChild(particleSystemElem);
		}
		modelElem.InsertEndChild(particleSystemsElem);
	}
}

void XMLModelDefinitionSerializer::exportBoneGroups(ModelDefinitionPtr modelDef, TiXmlElement& modelElem) {
	TiXmlElement boneGroupsElem("bonegroups");

	for (auto entry : modelDef->getBoneGroupDefinitions()) {
		TiXmlElement boneGroupElem("bonegroup");
		boneGroupElem.SetAttribute("name", entry.second->Name);

		TiXmlElement bonesElem("bones");
		for (auto boneIndex : entry.second->Bones) {
			TiXmlElement boneElem("bone");
			std::stringstream ss;
			ss << boneIndex;
			boneElem.SetValue(ss.str());
			bonesElem.InsertEndChild(boneElem);
		}
		boneGroupElem.InsertEndChild(bonesElem);

		boneGroupsElem.InsertEndChild(boneGroupElem);
	}
	modelElem.InsertEndChild(boneGroupsElem);
}



} //end namespace
}
}
