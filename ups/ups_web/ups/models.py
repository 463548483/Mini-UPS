# This is an auto-generated Django model module.
# You'll have to do the following manually to clean this up:
#   * Rearrange models' order
#   * Make sure each model has one field with primary_key=True
#   * Make sure each ForeignKey and OneToOneField has `on_delete` set to the desired behavior
#   * Remove `managed = False` lines if you wish to allow Django to create, modify, and delete the table
# Feel free to rename the models, but don't rename db_table values or field names.
from django.db import models
# from django.contrib.auth.models import User
from django.utils.translation import gettext_lazy as _

class Account(models.Model):
    accountid = models.AutoField(primary_key=True)
    username = models.CharField(max_length=40)

    class Meta:
        managed = False
        db_table = 'account'

class Item(models.Model):
    itemid = models.AutoField(primary_key=True)
    description = models.CharField(max_length=2000)
    amount = models.IntegerField()
    trackingnum = models.ForeignKey('Package', models.DO_NOTHING, db_column='trackingnum')

    class Meta:
        managed = False
        db_table = 'items'

class Truck(models.Model):
    truckid = models.AutoField(primary_key=True)
    class TruckStatus(models.TextChoices):
        IDLE = 'idle', _('The truck is idle')
        TRAVELING = 'traveling', _('The truck is traveling for picking up packages')
        ARRIVE_WAREHOUSE = 'arrive_warehouse', _('The truck arrived at warehouse')
        LOADING = 'loading', _('The truck is loading packages')
        DELIVERING = 'delivering', _('The truck is delivering ')
    status = models.CharField(
        max_length=20,
        choices=TruckStatus.choices,
        default=TruckStatus.IDLE,
    )
    x = models.IntegerField()
    y = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'trucks'

class Package(models.Model):
    trackingnum = models.AutoField(primary_key=True)
    destx = models.IntegerField(blank=True, null=True)
    desty = models.IntegerField(blank=True, null=True)
    truckid = models.ForeignKey(Truck, models.DO_NOTHING, db_column='truckid', blank=True, null=True)
    accountid = models.ForeignKey(Account, models.DO_NOTHING, db_column='accountid', blank=True, null=True)
    warehouseid = models.IntegerField()
    class PackageStatus(models.TextChoices):
        DELIVERED = 'delivered', _('The packaged has been delivered')
        DELIVERING = 'delivering', _('The package is under delivering')
        WAIT_FOR_LOADING = 'wait_for_loading', _('The package is waiting for loading')
        WAIT_FOR_PICKUP = 'wait_for_pickup', _('The package is waiting for pickup')
    status = models.CharField(
        max_length=20,
        choices=PackageStatus.choices,
        default=PackageStatus.WAIT_FOR_PICKUP,
    )

    class Meta:
        managed = False
        db_table = 'packages'


class Warehouse(models.Model):
    warehouseid = models.IntegerField(primary_key=True)
    x = models.IntegerField()
    y = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'warehouses'

class Searchhistory(models.Model):
    accountid = models.ForeignKey(Account, models.DO_NOTHING, db_column='accountid',unique=True)
    trackingnum = models.ForeignKey(Package, models.DO_NOTHING, db_column='trackingnum',unique=True)

    class Meta:
        managed = False
        db_table = 'searchhistory'
        unique_together = (('accountid', 'trackingnum'),)
