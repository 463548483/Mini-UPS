"""ups_web URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/4.0/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.urls import path, include
from ups.views import find_trucks_view, home_view, register_view, shipment_detail_view, track_shipment_view, my_packages_view, \
package_detail_view, address_change_view

from django.views.generic import TemplateView


urlpatterns = [
    path('admin/', admin.site.urls),
    path('', include('django.contrib.auth.urls')),
    path('', TemplateView.as_view(template_name="ups/index.html")),
    path('home', home_view, name='home'),
    path('register/', register_view, name='registration'),
    path('track_shipment/', track_shipment_view, name='tracking shipment'),
    path('track_shipment/<int:trackingnum>', shipment_detail_view, name='shipment_detail_view'),
    path('find_trucks', find_trucks_view, name='find_trucks'),
    path('my_packages/', my_packages_view, name='all my packages'),
    path('my_packages/<int:package_id>/detail/', package_detail_view, name='view package detail'),
    path('my_packages/<int:package_id>/change_address/', address_change_view, name='change package address'),
    # path('index', name='home'),
    
]
