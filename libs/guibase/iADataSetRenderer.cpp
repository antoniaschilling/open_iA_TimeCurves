#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iADataSet.h"
#include "iARenderer.h"

#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>

#include <QColor>

class iAOutlineImpl
{
public:
	iAOutlineImpl(iAAABB const& box, iARenderer* renderer, QColor const & c): m_renderer(renderer)
	{
		setBounds(box);
		m_mapper->SetInputConnection(m_cubeSource->GetOutputPort());
		m_actor->GetProperty()->SetRepresentationToWireframe();
		m_actor->GetProperty()->SetShading(false);
		m_actor->GetProperty()->SetOpacity(1);
		//m_actor->GetProperty()->SetLineWidth(2);
		m_actor->GetProperty()->SetAmbient(1.0);
		m_actor->GetProperty()->SetDiffuse(0.0);
		m_actor->GetProperty()->SetSpecular(0.0);
		setColor(c);
		m_actor->PickableOff();
		m_actor->SetMapper(m_mapper);
		renderer->renderer()->AddActor(m_actor);
	}
	void setVisible(bool visible)
	{
		if (visible)
		{
			m_renderer->renderer()->AddActor(m_actor);
		}
		else
		{
			m_renderer->renderer()->RemoveActor(m_actor);
		}
	}
	void setOrientationAndPosition(QVector<double> pos, QVector<double> ori)
	{
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_actor->SetPosition(pos.data());
		m_actor->SetOrientation(ori.data());
	}
	void setBounds(iAAABB const& box)
	{
		m_cubeSource->SetBounds(
			box.topLeft().x(), box.bottomRight().x(),
			box.topLeft().y(), box.bottomRight().y(),
			box.topLeft().z(), box.bottomRight().z()
		);
	}
	void setColor(QColor const& c)
	{
		m_actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	}
	QColor color() const
	{
		auto rgb = m_actor->GetProperty()->GetColor();
		return QColor(rgb[0], rgb[1], rgb[2]);
	}

private:
	vtkNew<vtkCubeSource> m_cubeSource;
	vtkNew<vtkPolyDataMapper> m_mapper;
	vtkNew<vtkActor> m_actor;
	iARenderer* m_renderer;
};

namespace
{
	const QString Position("Position");
	const QString Orientation("Orientation");
	const QString OutlineColor("Box Color");
	const QColor OutlineDefaultColor(Qt::black);
}

iADataSetRenderer::iADataSetRenderer(iARenderer* renderer):
	m_renderer(renderer),
	m_visible(false)
{
	addAttribute(Position, iAValueType::Vector3, QVariant::fromValue(QVector<double>({0, 0, 0})));
	addAttribute(Orientation, iAValueType::Vector3, QVariant::fromValue(QVector<double>({0, 0, 0})));
	addAttribute(OutlineColor, iAValueType::Color, OutlineDefaultColor);
}

void iADataSetRenderer::setAttributes(QMap<QString, QVariant> const & values)
{
	m_attribValues = values;
	applyAttributes(values);
	if (m_outline)
	{
		m_outline->setBounds(bounds());	// only potentially changes for volume currently; maybe use signals instead?
		m_outline->setOrientationAndPosition(
			m_attribValues[Position].value<QVector<double>>(), m_attribValues[Orientation].value<QVector<double>>());
		m_outline->setColor(m_attribValues[OutlineColor].value<QColor>());
	}
}

iAAttributes iADataSetRenderer::attributesWithValues() const
{
	iAAttributes result = combineAttributesWithValues(m_attributes, m_attribValues);
	// set position and orientation from current values:
	assert(result[0]->name() == Position);
	auto pos = position();
	result[0]->setDefaultValue(QVariant::fromValue(QVector<double>({pos[0], pos[1], pos[2]})));
	assert(result[1]->name() == Orientation);
	auto ori = orientation();
	result[1]->setDefaultValue(QVariant::fromValue(QVector<double>({ori[0], ori[1], ori[2]})));
	return result;
}

void iADataSetRenderer::setVisible(bool visible)
{
	m_visible = visible;
	if (m_visible)
	{
		showDataSet();
	}
	else
	{
		hideDataSet();
	}
}

bool iADataSetRenderer::isVisible() const
{
	return m_visible;
}

void iADataSetRenderer::setBoundsVisible(bool visible)
{
	if (!m_outline)
	{
		m_outline = std::make_shared<iAOutlineImpl>(bounds(), m_renderer,
			m_attribValues.contains(OutlineColor) ? m_attribValues[OutlineColor].value<QColor>() : OutlineDefaultColor);
		QVector<double> pos(3), ori(3);
		for (int i=0; i<3; ++i)
		{
			pos[i] = position()[i];
			ori[i] = orientation()[i];
		}
		m_outline->setOrientationAndPosition(pos, ori);
	}
	m_outline->setVisible(visible);
}

void iADataSetRenderer::addAttribute(
	QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
#ifndef _NDEBUG
	for (auto attr : m_attributes)
	{
		if (attr->name() == name)
		{
			LOG(lvlWarn, QString("iADataSetRenderer::addAttribute: Attribute with name %1 already exists!").arg(name));
		}
	}
#endif
	m_attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
	m_attribValues[name] = defaultValue;
}

// ---------- iAGraphRenderer ----------

#include <vtkGlyph3DMapper.h>

namespace
{
	const QString PointRadius = "Point Radius";
	const QString PointColor = "Point Color";
	const QString LineColor = "Line Color";
	const QString LineWidth = "Line Width";
}

class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(iARenderer* renderer, iAGraphData* data) :
		iADataSetRenderer(renderer),
		m_lineActor(vtkSmartPointer<vtkActor>::New()),
		m_pointActor(vtkSmartPointer<vtkActor>::New()),
		m_data(data)
	{
		auto lineMapper = vtkPolyDataMapper::New();
		lineMapper->SetInputData(data->poly());
		m_lineActor->SetMapper(lineMapper);
		m_lineActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

		// Glyph the points
		m_sphereSource = vtkSphereSource::New();
		m_sphereSource->SetPhiResolution(21);
		m_sphereSource->SetThetaResolution(21);
		m_sphereSource->SetRadius(5);
		vtkNew<vtkPoints> pointsPoints;
		pointsPoints->DeepCopy(data->poly()->GetPoints());
		vtkNew<vtkPolyData> glyphPoints;
		glyphPoints->SetPoints(pointsPoints);
		auto pointMapper = vtkGlyph3DMapper::New();
		pointMapper->SetInputData(glyphPoints);
		pointMapper->SetSourceConnection(m_sphereSource->GetOutputPort());
		m_pointActor->SetMapper(pointMapper);
		m_pointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

		addAttribute(PointRadius, iAValueType::Continuous, 5, 0.001, 100000000);
		addAttribute(PointColor, iAValueType::Color, "#FF0000");
		addAttribute(LineColor, iAValueType::Color, "#00FF00");
		addAttribute(LineWidth, iAValueType::Continuous, 1.0, 0.1, 100);
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddActor(m_lineActor);
		m_renderer->renderer()->AddActor(m_pointActor);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveActor(m_pointActor);
		m_renderer->renderer()->RemoveActor(m_lineActor);
	}

	void applyAttributes(QMap<QString, QVariant> const& values) override
	{
		m_sphereSource->SetRadius(values[PointRadius].toDouble());
		QColor pointColor(values[PointColor].toString());
		m_pointActor->GetProperty()->SetColor(pointColor.redF(), pointColor.greenF(), pointColor.blueF());
		m_sphereSource->Update();
		QColor lineColor(values[LineColor].toString());
		m_lineActor->GetProperty()->SetColor(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
		m_lineActor->GetProperty()->SetLineWidth(values[LineWidth].toFloat());

		QVector<double> pos = values[Position].value<QVector<double>>();
		QVector<double> ori = values[Orientation].value<QVector<double>>();
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_pointActor->SetPosition(pos.data());
		m_pointActor->SetOrientation(ori.data());
		m_lineActor->SetPosition(pos.data());
		m_lineActor->SetOrientation(ori.data());
	}

private:
	iAAABB bounds() override
	{
		return iAAABB(m_data->poly()->GetBounds());
	}
	double const* orientation() const override
	{
		auto o1 = m_pointActor->GetOrientation(), o2 = m_lineActor->GetOrientation();
		assert(o1[0] == o2[0] && o1[1] == o2[1] && o1[2] == o2[2]);
		return m_pointActor->GetOrientation();
	}
	double const* position() const override
	{
		auto p1 = m_pointActor->GetPosition(), p2 = m_lineActor->GetPosition();
		assert(p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2]);
		return m_lineActor->GetPosition();
	}
	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
	iAGraphData* m_data;
};



// ---------- iAMeshRenderer ----------

namespace
{
	const QString PolyColor = "Color";
	const QString PolyOpacity = "Opacity";
}

class iAMeshRenderer: public iADataSetRenderer
{
public:
	iAMeshRenderer(iARenderer* renderer, iAPolyData * data):
		iADataSetRenderer(renderer),
		m_polyActor(vtkSmartPointer<vtkActor>::New()),
		m_data(data)
	{
		vtkNew<vtkPolyDataMapper> mapper;
		mapper->SetInputData(data->poly());
		//m_polyMapper->SelectColorArray("Colors");
		mapper->SetScalarModeToUsePointFieldData();
		m_polyActor->SetPickable(false);
		m_polyActor->SetDragable(false);
		m_polyActor->SetMapper(mapper);

		addAttribute(PolyColor, iAValueType::Color, "#FFFFFF");
		addAttribute(PolyOpacity, iAValueType::Continuous, 1.0, 0.0, 1.0);
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddActor(m_polyActor);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveActor(m_polyActor);
	}
	void applyAttributes(QMap<QString, QVariant> const& values) override
	{
		QColor color(values[PolyColor].toString());
		double opacity = values[PolyOpacity].toDouble();
		m_polyActor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
		m_polyActor->GetProperty()->SetOpacity(opacity);

		QVector<double> pos = values[Position].value<QVector<double>>();
		QVector<double> ori = values[Orientation].value<QVector<double>>();
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_polyActor->SetPosition(pos.data());
		m_polyActor->SetOrientation(ori.data());
	}

private:
	iAAABB bounds() override
	{
		return iAAABB(m_data->poly()->GetBounds());
	}
	double const* orientation() const override
	{
		return m_polyActor->GetOrientation();
	}
	double const* position() const override
	{
		return m_polyActor->GetPosition();
	}
	vtkSmartPointer<vtkActor> m_polyActor;
	iAPolyData* m_data;
};

// ---------- iAVolRenderer ----------
#include "iAModalityTransfer.h"
#include "iAToolsVTK.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>

namespace
{
	const QString LinearInterpolation = "Linear interpolation";
	const QString Shading = "Shading";
	const QString AmbientLighting = "Ambient lighting";
	const QString DiffuseLighting= "Diffuse lighting";
	const QString SpecularLighting = "Specular lighting";
	const QString SpecularPower = "Specular power";
	const QString ScalarOpacityUnitDistance = "Scalar Opacity Unit Distance";
	const QString RendererType = "Renderer type";
	const QString Spacing = "Spacing";
	const QString InteractiveAdjustSampleDistance = "Interactively Adjust Sample Distances";
	const QString AutoAdjustSampleDistance = "Auto-Adjust Sample Distances";
	const QString SampleDistance = "Sample distance";
	const QString InteractiveUpdateRate = "Interactive Update Rate";
	const QString FinalColorLevel = "Final Color Level";
	const QString FinalColorWindow = "Final Color Window";
	// VTK 9.2
	//const QString GlobalIlluminationReach = "Global Illumination Reach";
	//const QString VolumetricScatteringBlending = "VolumetricScatteringBlending";
}

class iAVolRenderer: public iADataSetRenderer
{
public:
	iAVolRenderer(iARenderer* renderer, iAImageData* data) :
		iADataSetRenderer(renderer),
		m_volume(vtkSmartPointer<vtkVolume>::New()),
		m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
		m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
		m_transfer(std::make_shared<iAModalityTransfer>(data->image()->GetScalarRange())),
		m_image(data)
	{
		m_volMapper->SetBlendModeToComposite();
		m_volume->SetMapper(m_volMapper);
		m_volume->SetProperty(m_volProp);
		m_volume->SetVisibility(true);
		m_volMapper->SetInputData(data->image());
		if (data->image()->GetNumberOfScalarComponents() > 1)
		{
			m_volMapper->SetBlendModeToComposite();
			m_volProp->IndependentComponentsOff();
		}
		else
		{
			m_volProp->SetColor(0, m_transfer->colorTF());
		}
		m_volProp->SetScalarOpacity(0, m_transfer->opacityTF());
		m_volProp->Modified();

		// volume properties:
		iAVolumeSettings volumeSettings;	// get global default settings?
		addAttribute(LinearInterpolation, iAValueType::Boolean, volumeSettings.LinearInterpolation);
		addAttribute(Shading, iAValueType::Boolean, volumeSettings.Shading);
		addAttribute(AmbientLighting, iAValueType::Continuous, volumeSettings.AmbientLighting);
		addAttribute(DiffuseLighting, iAValueType::Continuous, volumeSettings.DiffuseLighting);
		addAttribute(SpecularLighting, iAValueType::Continuous, volumeSettings.SpecularLighting);
		addAttribute(SpecularPower, iAValueType::Continuous, volumeSettings.SpecularPower);
		addAttribute(ScalarOpacityUnitDistance, iAValueType::Continuous, volumeSettings.ScalarOpacityUnitDistance);

		// mapper properties:
		QStringList renderTypes = RenderModeMap().values();
		selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
		addAttribute(RendererType, iAValueType::Categorical, renderTypes);
		addAttribute(InteractiveAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(AutoAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(SampleDistance, iAValueType::Continuous, volumeSettings.SampleDistance);
		addAttribute(InteractiveUpdateRate, iAValueType::Continuous, 1.0);
		addAttribute(FinalColorLevel, iAValueType::Continuous, 0.5);
		addAttribute(FinalColorWindow, iAValueType::Continuous, 1.0);
		// -> VTK 9.2 ?
		//addAttribute(GlobalIlluminationReach, iAValueType::Continuous, 0.0, 0.0, 1.0);
		//addAttribute(VolumetricScatteringBlending, iAValueType::Continuous, -1.0, 0.0, 2.0);

		// volume properties:
		auto spc = data->image()->GetSpacing();
		QVector<double> spacing({spc[0], spc[1], spc[2]});
		addAttribute(Spacing, iAValueType::Vector3, QVariant::fromValue(spacing));

		applyAttributes(m_attribValues);  // addAttribute adds default values; apply them now!
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddVolume(m_volume);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveVolume(m_volume);
	}
	void applyAttributes(QMap<QString, QVariant> const& values) override
	{
		m_volProp->SetAmbient(values[AmbientLighting].toDouble());
		m_volProp->SetDiffuse(values[DiffuseLighting].toDouble());
		m_volProp->SetSpecular(values[SpecularLighting].toDouble());
		m_volProp->SetSpecularPower(values[SpecularPower].toDouble());
		m_volProp->SetInterpolationType(values[LinearInterpolation].toInt());
		m_volProp->SetShade(values[Shading].toBool());
		if (values[ScalarOpacityUnitDistance].toDouble() > 0)
		{
			m_volProp->SetScalarOpacityUnitDistance(values[ScalarOpacityUnitDistance].toDouble());
		}
		/*
		else
		{
			m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
		}
		*/
		m_volMapper->SetRequestedRenderMode(values[RendererType].toInt());
		m_volMapper->SetInteractiveAdjustSampleDistances(values[InteractiveAdjustSampleDistance].toBool());
		m_volMapper->SetAutoAdjustSampleDistances(values[AutoAdjustSampleDistance].toBool());
		m_volMapper->SetSampleDistance(values[SampleDistance].toDouble());
		m_volMapper->SetInteractiveUpdateRate(values[InteractiveUpdateRate].toDouble());
		m_volMapper->SetFinalColorLevel(values[FinalColorLevel].toDouble());
		m_volMapper->SetFinalColorWindow(values[FinalColorWindow].toDouble());
		// VTK 9.2:
		//m_volMapper->SetGlobalIlluminationReach
		//m_volMapper->SetVolumetricScatteringBlending

		QVector<double> pos = values[Position].value<QVector<double>>();
		QVector<double> ori = values[Orientation].value<QVector<double>>();
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_volume->SetPosition(pos.data());
		m_volume->SetOrientation(ori.data());
		
		QVector<double> spc = values[Spacing].value<QVector<double>>();
		assert(spc.size() == 3);
		m_image->image()->SetSpacing(spc.data());
	}
	/*
	void setMovable(bool movable) override
	{
		m_volume->SetPickable(movable);
		m_volume->SetDragable(movable);
	}
	*/
	

private:
	iAAABB bounds() override
	{
		return iAAABB(m_image->image()->GetBounds());
	}
	double const* orientation() const override
	{
		return m_volume->GetOrientation();
	}
	double const* position() const override
	{
		return m_volume->GetPosition();
	}
	//iAVolumeSettings m_volSettings;
	std::shared_ptr<iAModalityTransfer> m_transfer;

	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
	iAImageData* m_image;
};

// ---------- Factory method ----------

std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer)
{
	switch(dataset->type())
	{
	case iADataSetType::dstVolume:
		return std::make_shared<iAVolRenderer>(renderer, dynamic_cast<iAImageData*>(dataset));
	case iADataSetType::dstGraph:
		return std::make_shared<iAGraphRenderer>(renderer, dynamic_cast<iAGraphData*>(dataset));
	case iADataSetType::dstMesh:
		return std::make_shared<iAMeshRenderer>(renderer, dynamic_cast<iAPolyData*>(dataset));

	default:
		LOG(lvlWarn, QString("Requested renderer for unknown type %1!")
			.arg(dataset->type()));
		return {};
	}
}