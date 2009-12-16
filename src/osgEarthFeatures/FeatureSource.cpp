/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2009 Pelican Ventures, Inc.
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <osgEarthFeatures/FeatureSource>
#include <osgEarthFeatures/ResampleFilter>
#include <osgEarthFeatures/BufferFilter>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <OpenThreads/ScopedLock>

using namespace osgEarth::Features;
using namespace OpenThreads;

#define PROP_RESAMPLE_OP         "resample"
#define RESAMPLE_ATTR_MIN_LENGTH "min_length"
#define RESAMPLE_ATTR_MAX_LENGTH "max_length"

#define PROP_BUFFER_OP           "buffer"
#define BUFFER_ATTR_DISTANCE     "distance"

/****************************************************************************/

FeatureSource::FeatureSource( const PluginOptions* options ) :
_options( options )
{    
    const Config& conf = getOptions()->config();

    // optional feature operations
    // TODO: at some point, move all this stuff elsewhere into some sort of filter
    // pipeline manager

    // resample operation:
    if ( conf.hasChild( PROP_RESAMPLE_OP ) )
    {
        ResampleFilter* resample = new ResampleFilter();
        resample->minLength() = conf.child( PROP_RESAMPLE_OP ).value<double>( RESAMPLE_ATTR_MIN_LENGTH, resample->minLength() );
        resample->maxLength() = conf.child( PROP_RESAMPLE_OP ).value<double>( RESAMPLE_ATTR_MAX_LENGTH, resample->maxLength() );
        _filters.push_back( resample );
    }

    // buffer operation:
    if ( conf.hasChild( PROP_BUFFER_OP ) )
    {
        BufferFilter* buffer = new BufferFilter();
        buffer->distance() = conf.child( PROP_BUFFER_OP ).value<double>( BUFFER_ATTR_DISTANCE, 1.0 );
        _filters.push_back( buffer );
    }
}

FeatureSource::~FeatureSource()
{
    //nop
}

const PluginOptions* 
FeatureSource::getOptions() const
{
    return _options.get();
}

const FeatureProfile*
FeatureSource::getFeatureProfile() const
{
    if ( !_featureProfile.valid() )
    {
        // caching pattern
        FeatureSource* nonConstThis = const_cast<FeatureSource*>(this);
        nonConstThis->_featureProfile = nonConstThis->createFeatureProfile();

        //// apply a geometry type override.
        //if ( _geomTypeOverride != Geometry::TYPE_UNKNOWN )
        //{
        //    nonConstThis->_featureProfile = new FeatureProfile(
        //        _featureProfile->getSRS(),
        //        _geomTypeOverride );

        //    //nonConstThis->_featureProfile = new FeatureProfile(
        //    //    _featureProfile->getSRS(),
        //    //    _geomTypeOverride,
        //    //    _featureProfile->getDimensionality(),
        //    //    _featureProfile->isMultiGeometry() );
        //}
    }
    return _featureProfile.get();
}

const GeoExtent&
FeatureSource::getDataExtent() const
{
    if ( _dataExtent.defined() )
    {
        // caching pattern
        FeatureSource* nonConstThis = const_cast<FeatureSource*>(this);
        nonConstThis->_dataExtent = nonConstThis->createDataExtent();
    }
    return _dataExtent;
}

//const Geometry::Type&
//FeatureSource::getGeometryTypeOverride() const
//{
//    return _geomTypeOverride;
//}

//void 
//FeatureSource::setGeometryTypeOverride( const Geometry::Type& type )
//{
//    _geomTypeOverride = type;
//}

FeatureFilterList&
FeatureSource::getFilters()
{
    return _filters;
}

/****************************************************************************/

FeatureSource*
FeatureSourceFactory::create(const std::string& name,
                             const std::string& driver,
                             const Config&      driverConf,
                             const osgDB::ReaderWriter::Options* globalOptions )
{
    osg::ref_ptr<PluginOptions> options = globalOptions?
        new PluginOptions( *globalOptions ) :
        new PluginOptions();

    //Setup the plugin options for the source
    options->config() = driverConf;

    osg::notify(osg::INFO)
        << "[osgEarth] Feature Driver " << driver << ", config =" << std::endl << driverConf.toString() << std::endl;

	// Load the source from a plugin.
    osg::ref_ptr<FeatureSource> source = dynamic_cast<FeatureSource*>(
                osgDB::readObjectFile( ".osgearth_feature_" + driver, options.get()));

    if ( source.valid() )
    {
        source->setName( name );
    }
    else
	{
		osg::notify(osg::NOTICE) << "[osgEarth] Warning: Could not load Feature Source for driver "  << driver << std::endl;
	}

	return source.release();
}


FeatureSource*
FeatureSourceFactory::create(const Config& featureStoreConf,
                             const osgDB::ReaderWriter::Options* globalOptions )
{
    return create(
        featureStoreConf.attr( "name" ),
        featureStoreConf.attr( "driver" ),
        featureStoreConf,
        globalOptions );
}

