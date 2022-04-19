# from asyncio.windows_events import NULL
import json
import socket
import threading
from django.shortcuts import render, redirect
from django.contrib import messages
from requests import request
from .forms import RegisterForm
from .models import Account, Package, Truck, Item, Searchhistory

BACKEND_HOST = "127.0.0.1"
BACKEND_PORT = 5555 

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
        Searchhistory.objects.create(accountid=request.user.pk, trackingnum=track_num)

        message = "Found %s results." % (len(packages))  
        context = {'packages': packages, 'message': message}  
    else:
        # get all packages
        searchhistory = Searchhistory.objects.filter(
            accountid=request.user.pk,
        ).order_by('trackingnum')
        packages=''
        message = "You have %s records search history." % (len(searchhistory))
        context = {'searchhistory': searchhistory, 'message': message}
    # context = {'packages': packages,'searchhistory': searchhistory, 'message': message}
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
        tracking_num = kwargs['package_id'] 
        truck_id = Package.objects.get(trackingnum = tracking_num).truckid.truckid
        
        # request backend to change address
        # t = threading.Thread(target=RequestBackendToChangeAddress, args=(request, tracking_num, new_x, new_y, truck_id,))
        # t.start()
        RequestBackendToChangeAddress(request, tracking_num, new_x, new_y, truck_id)

        redirect_path = '/my_packages'
        return redirect(redirect_path)
    else:
        package = Package.objects.get(trackingnum = kwargs['package_id'])
        dest_x = package.destx
        dest_y = package.desty
    
        context = {'dest_x': dest_x, 'dest_y': dest_y}
        return render(request, 'ups/address_change.html', context=context)

def RequestBackendToChangeAddress(request, tracking_num, new_x, new_y, truck_id):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        print(tracking_num, new_x, new_y, truck_id)
        try:
            s.connect((BACKEND_HOST, BACKEND_PORT))
            print("sending message to backend")
            change_request = {"request": "Change Address",
                              "tracking_num": tracking_num, 
                              "new_x": new_x,
                              "new_y": new_y,
                              "truck_id": truck_id}
            change_request_json = json.dumps(change_request)
            s.sendall(bytes(change_request_json, encoding="utf-8"))
            
            result_raw = s.recv(1024)
            print("Received result from backend")
            result = json.loads(result_raw)
            if result['result'] == 'failure':
                # notify user of failure
                messages.error(request, result['error'])
            elif result['result'] == 'success':
                # notify user of success
                messages.success(request, "Address changed successfully")
        except:
            print("Something went wrong while communicating with backend")
        