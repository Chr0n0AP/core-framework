/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file 
 * distributed with this source distribution.
 * 
 * This file is part of REDHAWK core.
 * 
 * REDHAWK core is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * REDHAWK core is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#include "Placement.h"

using namespace ossie;

PlacementPlan::PlacementPlan()
{
}

PlacementPlan::PlacementPlan(const std::string& id, const std::string& name) :
    id(id),
    name(name)
{
}

const std::string& PlacementPlan::getId() const
{
    return id;
}

const std::string& PlacementPlan::getName() const
{
    return name;
}

const PlacementPlan::ComponentList& PlacementPlan::getComponents() const
{
    return components;
}

void PlacementPlan::addComponent(ComponentInfo* component)
{
    components.push_back(component);
}

ComponentInfo* PlacementPlan::getComponent(const std::string& instantiationId)
{
    for (ComponentList::iterator comp = components.begin(); comp != components.end(); ++comp) {
        if (instantiationId == (*comp)->getInstantiationIdentifier()) {
            return *comp;
        }
    }

    return 0;
}

ApplicationPlacement::ApplicationPlacement()
{
}

ApplicationPlacement::~ApplicationPlacement()
{
    for (std::vector<PlacementPlan*>::iterator place = placements.begin(); place != placements.end(); ++place) {
        delete (*place);
    }
}

void ApplicationPlacement::addPlacement(PlacementPlan* placement)
{
    placements.push_back(placement);
}

const std::vector<PlacementPlan*>& ApplicationPlacement::getPlacements() const
{
    return placements;
}

ComponentInfo* ApplicationPlacement::getComponent(const std::string& instantiationId)
{
    for (PlacementList::iterator placement = placements.begin(); placement != placements.end(); ++placement) {
        ComponentInfo* component = (*placement)->getComponent(instantiationId);
        if (component) {
            return component;
        }
    }

    return 0;
}

ComponentInfo* ApplicationPlacement::getAssemblyController()
{
    for (PlacementList::iterator placement = placements.begin(); placement != placements.end(); ++placement) {
        const std::vector<ComponentInfo*>& components = (*placement)->getComponents();
        for (std::vector<ComponentInfo*>::const_iterator comp = components.begin(); comp != components.end(); ++comp) {
            if ((*comp)->isAssemblyController()) {
                return *comp;
            }
        }
    }

    return 0;
}
