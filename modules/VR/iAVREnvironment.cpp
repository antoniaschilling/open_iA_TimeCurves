/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAVREnvironment.h"

#include "iAVRInteractor.h"

#include <iALog.h>

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>
#include <vtkPNGReader.h>
#include <vtkImageFlip.h>
#include <vtkLight.h>
#include <vtkLightKit.h>
#include <vtkPickingManager.h>

#include <qstring.h>
#include <QCoreApplication>

#include <QThread>

class iAVRMainThread : public QThread
{
public:
	iAVRMainThread(vtkSmartPointer<vtkOpenVRRenderer> ren, vtkSmartPointer<vtkOpenVRRenderWindow> renderWindow, vtkSmartPointer<vtkOpenVRRenderWindowInteractor> interactor):
		m_renderer(ren), m_renderWindow(renderWindow), m_interactor(interactor)
	{}
	void run() override
	{
		m_renderWindow->Render();
		m_interactor->Start();
	}
	void stop()
	{
		m_interactor->SetDone(true);
		//m_renderWindow->Finalize();
	}
private:
	vtkSmartPointer<vtkOpenVRRenderer> m_renderer;
	vtkSmartPointer<vtkOpenVRRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkOpenVRRenderWindowInteractor> m_interactor;
};




iAVREnvironment::iAVREnvironment():	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()), m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New()), m_interactor(vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New()),
	m_vrMainThread(nullptr)
{	
	createSkybox(0);
	createLightKit();
	//m_renderer->SetShowFloor(true);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

vtkOpenVRRenderWindowInteractor* iAVREnvironment::interactor()
{
	return m_interactor;
}

vtkOpenVRRenderWindow* iAVREnvironment::renderWindow()
{
	return m_renderWindow;
}

void iAVREnvironment::update()
{
	m_renderWindow->Render();
}

void iAVREnvironment::start()
{
	if (m_vrMainThread)
	{
		LOG(lvlWarn, "Cannot start more than one VR session in parallel!");
		emit finished();
		return;
	}
	m_renderWindow->AddRenderer(m_renderer);
	// MultiSamples needs to be set to 0 to make Volume Rendering work:
	// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
	m_renderWindow->SetMultiSamples(0);
	m_interactor->SetRenderWindow(m_renderWindow);
	auto camera = vtkSmartPointer<vtkOpenVRCamera>::New();

	m_renderer->SetActiveCamera(camera);
	m_renderer->ResetCamera();
	m_renderer->ResetCameraClippingRange();
	m_interactor->GetPickingManager()->EnabledOn();

	m_vrMainThread = new iAVRMainThread(m_renderer, m_renderWindow, m_interactor);
	connect(m_vrMainThread, &QThread::finished, this, &iAVREnvironment::vrDone);
	m_vrMainThread->setObjectName("vrMainThread");
	m_vrMainThread->start();
	//TODO: Wait for thread to finish or the rendering might not have started yet
	storeInitialWorldScale();
	//emit finished();
}

void iAVREnvironment::stop()
{
	if (!m_vrMainThread)
	{
		LOG(lvlWarn, "VR Environment not running!");
		return;
	}
	if (m_vrMainThread)
	{
		m_vrMainThread->stop();
		emit finished();
	}
		
}

void iAVREnvironment::createLightKit()
{
	vtkSmartPointer<vtkLightKit> light = vtkSmartPointer<vtkLightKit>::New();
	light->SetKeyLightIntensity(0.88);
	light->AddLightsToRenderer(m_renderer);
}

double iAVREnvironment::getInitialWorldScale()
{
	return m_worldScale;
}

void iAVREnvironment::storeInitialWorldScale()
{
	m_worldScale = m_interactor->GetPhysicalScale();
}

void iAVREnvironment::createSkybox(int skyboxImage)
{
	//const std::string chosenSkybox = QString("/skybox%1/").arg(skyboxImage).toUtf8();
	const std::string chosenSkybox = "/skybox" + std::to_string(skyboxImage) + "/";

	// Load the skybox
	// Read it again as there is no deep copy for vtkTexture
	QString path = QCoreApplication::applicationDirPath() + "/VR-skybox";
	auto skybox = ReadCubeMap(path.toStdString(), chosenSkybox, ".png", 0);
	skybox->InterpolateOn();
	skybox->RepeatOff();
	skybox->EdgeClampOn();

	//m_renderer->UseImageBasedLightingOn();
	//m_renderer->SetEnvironmentTexture(cubemap);

	skyboxActor = vtkSmartPointer<vtkSkybox>::New();
	skyboxActor->SetTexture(skybox);
	m_renderer->AddActor(skyboxActor);
}

vtkSmartPointer<vtkTexture> iAVREnvironment::ReadCubeMap(std::string const& folderPath,
	std::string const& fileRoot,
	std::string const& ext, int const& key)
{
	// A map of cube map naming conventions and the corresponding file name
	// components.
	std::map<int, std::vector<std::string>> fileNames{
		{0, {"right", "left", "top", "bottom", "front", "back"}},
		{1, {"posx", "negx", "posy", "negy", "posz", "negz"}},
		{2, {"-px", "-nx", "-py", "-ny", "-pz", "-nz"}},
		{3, {"0", "1", "2", "3", "4", "5"}} };
	std::vector<std::string> fns;
	if (fileNames.count(key))
	{
		fns = fileNames.at(key);
	}
	else
	{
		std::cerr << "ReadCubeMap(): invalid key, unable to continue." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto texture = vtkSmartPointer<vtkTexture>::New();
	texture->CubeMapOn();
	// Build the file names.
	std::for_each(fns.begin(), fns.end(),
		[&folderPath, &fileRoot, &ext](std::string& f) {
		f = folderPath + fileRoot + f + ext;
	});
	auto i = 0;
	for (auto const& fn : fns)
	{
		auto imgReader = vtkSmartPointer<vtkPNGReader>::New();
		imgReader->SetFileName(fn.c_str());

		auto flip = vtkSmartPointer<vtkImageFlip>::New();
		flip->SetInputConnection(imgReader->GetOutputPort());
		flip->SetFilteredAxis(1); // flip y axis
		texture->SetInputConnection(i, flip->GetOutputPort(0));
		++i;
	}
	return texture;
}

bool iAVREnvironment::isRunning() const
{
	return m_vrMainThread;
}

void iAVREnvironment::vrDone()
{
	delete m_vrMainThread;
	m_vrMainThread = nullptr;
}

