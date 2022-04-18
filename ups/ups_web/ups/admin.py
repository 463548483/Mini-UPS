from django.contrib import admin

# Register your models here.
from django.contrib import admin
from .models import Account, Item, Package, Truck

admin.site.register(Account)
admin.site.register(Item)
admin.site.register(Package)
admin.site.register(Truck)