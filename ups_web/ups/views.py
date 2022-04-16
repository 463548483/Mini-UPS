# from asyncio.windows_events import NULL
from django.shortcuts import render, redirect
from .forms import RegisterForm
from .models import Account, Package, Truck, Item

# Create your views here.
def home_view(request, *args, **kwargs):
    user = request.user
    introduction = 'Testing homepage'
    
    context = {'introduction': introduction, 'user': user}
    return render(request, 'ups/home.html', context=context)

def register_view(request, *args, **kwargs):
    register_message = "UPS Account Registration"
    form = RegisterForm()
    if request.method == "POST":
        form = RegisterForm(request.POST)
        if form.is_valid():
            user = form.save()
            Account.objects.create(accountid = user.pk, username = user.username)

            return redirect("/login")
        else:
            print(form.errors)

    context = {
        'register_message': register_message,
        'form': form,
    }
    return render(request, 'ups/register.html', context=context)

def track_shipment_view(request, *args, **kwargs):
    if request.method == 'POST':
        track_num = int(request.POST.get('tracking_number'))

        packages = Package.objects.filter(
            trackingnum = track_num,
        ).order_by('trackingnum') 

        message = "Found %s results." % (len(packages))    
    else:
        # get all packages
        packages = Package.objects.filter().order_by('trackingnum')

        message = "Overview of all shipments."

    context = {'packages': packages, 'message': message}
    return render(request, 'ups/track_shipment.html', context=context)

def my_packages_view(request, *args, **kwargs):
    packages = Package.objects.filter(
        accountid = request.user.pk,
    ).order_by('trackingnum') 
    
    context = {'packages': packages}
    return render(request, 'ups/my_packages.html', context=context)

def package_detail_view(request, *args, **kwargs):
    items = Item.objects.filter(
        trackingnum = kwargs['package_id']
    ).order_by('itemid')
    
    context = {'items': items}
    return render(request, 'ups/package_detail.html', context=context)

def address_change_view(request, *args, **kwargs):
    if request.method == 'POST':
        new_x = int(request.POST.get('new_x'))
        new_y = int(request.POST.get('new_y'))
        print('Get new address (%d, %d)' % (new_x, new_y))
        # request backend to change address

        redirect_path = '/my_packages'
        return redirect(redirect_path)
    else:
        package = Package.objects.get(trackingnum = kwargs['package_id'])
        dest_x = package.destx
        dest_y = package.desty
    
        context = {'dest_x': dest_x, 'dest_y': dest_y}
        return render(request, 'ups/address_change.html', context=context)

